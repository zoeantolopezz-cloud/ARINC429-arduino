/*
 * Lector básico de palabras ARINC 429 por puerto serie
 * Compatible con Arduino UNO y simulación Wokwi.
 *
 * Entrada serie: una línea por palabra.
 * Formatos aceptados: 32 caracteres '0'/'1' o 0x/0X seguido de 8 dígitos hex.
 *
 * FORMATO SERIE PENDIENTE: el repositorio no contiene el proyecto anterior.
 * Si ese proyecto usaba otro separador o protocolo, hay que reemplazar leerLineaSerie()
 * antes de usar el sistema real. La palabra de 32 bits sigue siendo la misma.
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <string.h>

// LCD I2C usado por el ejemplo Wokwi. Se puede cambiar si la placa real usa otro modelo.
LiquidCrystal_I2C lcd(0x27, 16, 2);

const uint8_t PIN_BIT_1 = 13;
const uint8_t PIN_BIT_0 = 12;
const unsigned long BAUD_RATE = 9600;

bool leerLineaSerie(char *linea, uint8_t tamanoMaximo);
bool textoA32Bits(const char *texto, uint32_t &palabra);
uint32_t numeroHex32(const char *texto, bool &valido);
uint8_t reverso8(uint8_t valor);
bool tiene32Bits(uint32_t palabra);
bool tieneParidadImpar(uint32_t palabra);
void reproducirBitsEnPines(uint32_t palabra);
void apagarPines();
void mostrarError(const char *mensaje);

void setup() {
  pinMode(PIN_BIT_1, OUTPUT);
  pinMode(PIN_BIT_0, OUTPUT);
  digitalWrite(PIN_BIT_1, LOW);
  digitalWrite(PIN_BIT_0, LOW);

  Serial.begin(BAUD_RATE);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ARINC 429 listo");
  lcd.setCursor(0, 1);
  lcd.print("Esperando palabra");

  Serial.println("Arduino UNO - lector ARINC 429");
  Serial.println("Envie 32 bits (0/1) o 0x seguido de 8 digitos hex");
}

void loop() {
  char linea[65];

  if (!leerLineaSerie(linea, sizeof(linea))) {
    return;
  }

  uint32_t palabra;
  if (!textoA32Bits(linea, palabra)) {
    mostrarError("Formato invalido");
    Serial.println("ERROR: use 32 caracteres 0/1 o 0x + 8 digitos hex");
    apagarPines();
    return;
  }

  if (!tiene32Bits(palabra)) {
    // La conversión anterior ya limita el dato a 32 bits; se mantiene como control.
    mostrarError("Palabra != 32b");
    Serial.println("ERROR: la palabra no tiene 32 bits");
    apagarPines();
    return;
  }

  bool paridadOK = tieneParidadImpar(palabra);
  if (!paridadOK) {
    mostrarError("Paridad incorrecta");
    Serial.print("ERROR: paridad incorrecta; palabra=0x");
    Serial.println(palabra, HEX);
    apagarPines();
    return;
  }

  // El Label ocupa los bits 1 a 8 (indices 0 a 7).
  // ARINC suele representar cada byte de Label en orden de bits invertido.
  uint8_t labelEnPalabra = palabra & 0xFF;
  uint8_t labelDecimal = reverso8(labelEnPalabra);
  uint16_t labelOctal = labelDecimal; // Se imprime en formato octal de tres digitos.

  // Campo de datos: bits 12 a 29 (indices 11 a 29), inclusive.
  uint32_t valor = (palabra >> 11) & 0x0007FFFFUL;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Label ");
  lcd.print(labelOctal, OCT);

  lcd.setCursor(0, 1);
  lcd.print("Valor ");
  if (valor <= 999999UL) {
    lcd.print(valor);
  } else {
    lcd.print("H");
    lcd.print(valor, HEX);
  }

  Serial.print("Label octal=");
  Serial.print(labelOctal, OCT);
  Serial.print(" valor=");
  Serial.print(valor);
  Serial.print(" paridad=OK palabra=0x");
  Serial.println(palabra, HEX);

  reproducirBitsEnPines(palabra);
}

bool leerLineaSerie(char *linea, uint8_t tamanoMaximo) {
  if (!Serial.available()) {
    return false;
  }

  size_t longitud = Serial.readBytesUntil('\n', linea, tamanoMaximo - 1);
  linea[longitud] = '\0';

  // Permite CRLF y elimina espacios al inicio/final.
  while (longitud > 0 && (linea[longitud - 1] == '\r' || linea[longitud - 1] == ' ')) {
    linea[--longitud] = '\0';
  }

  size_t inicio = 0;
  while (linea[inicio] == ' ') {
    inicio++;
  }
  if (inicio > 0) {
    memmove(linea, linea + inicio, strlen(linea + inicio) + 1);
  }

  return strlen(linea) > 0;
}

bool textoA32Bits(const char *texto, uint32_t &palabra) {
  size_t longitud = strlen(texto);

  if (longitud == 32) {
    palabra = 0;
    for (size_t i = 0; i < 32; i++) {
      if (texto[i] == '1') {
        palabra |= (1UL << (31 - i));
      } else if (texto[i] != '0') {
        return false;
      }
    }
    return true;
  }

  if (longitud == 10 && (texto[0] == '0') && (texto[1] == 'x' || texto[1] == 'X')) {
    bool valido;
    palabra = numeroHex32(texto + 2, valido);
    return valido; // Validación real en el bucle siguiente.
  }

  return false;
}

uint32_t numeroHex32(const char *texto, bool &valido) {
  uint32_t resultado = 0;
  valido = true;
  for (uint8_t i = 0; i < 8; i++) {
    char c = texto[i];
    uint8_t digito;
    if (c >= '0' && c <= '9') digito = c - '0';
    else if (c >= 'A' && c <= 'F') digito = c - 'A' + 10;
    else if (c >= 'a' && c <= 'f') digito = c - 'a' + 10;
    else {
      valido = false;
      return 0;
    }
    resultado = (resultado << 4) | digito;
  }
  return resultado;
}

bool tiene32Bits(uint32_t palabra) {
  // Los valores uint32_t tienen siempre hasta 32 bits. Esta función documenta el control.
  (void)palabra;
  return true;
}

bool tieneParidadImpar(uint32_t palabra) {
  uint8_t unos = 0;
  for (uint8_t i = 0; i < 32; i++) {
    unos += (palabra >> i) & 1U;
  }
  return (unos % 2) == 1;
}

uint8_t reverso8(uint8_t valor) {
  uint8_t resultado = 0;
  for (uint8_t i = 0; i < 8; i++) {
    resultado |= ((valor >> i) & 1U) << (7 - i);
  }
  return resultado;
}

void reproducirBitsEnPines(uint32_t palabra) {
  for (int8_t i = 31; i >= 0; i--) {
    uint32_t bit = (palabra >> i) & 1U;

    if (bit == 1) {
      digitalWrite(PIN_BIT_1, HIGH);
      digitalWrite(PIN_BIT_0, LOW);
    } else {
      digitalWrite(PIN_BIT_1, LOW);
      digitalWrite(PIN_BIT_0, HIGH);
    }

    delay(10);
  }

  // Después de recorrer los 32 bits deja ambos pines activos en nivel bajo.
  apagarPines();
}

void apagarPines() {
  digitalWrite(PIN_BIT_1, LOW);
  digitalWrite(PIN_BIT_0, LOW);
}

void mostrarError(const char *mensaje) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ERROR");
  lcd.setCursor(0, 1);
  lcd.print(mensaje);
}
