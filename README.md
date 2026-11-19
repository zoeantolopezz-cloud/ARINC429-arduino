# ARINC 429 con Arduino UNO

Programa de demostración para leer palabras ARINC 429 por puerto serie, validar su longitud/paridad y mostrar el Label y el valor en una pantalla LCD I2C 16x2.

## Componentes utilizados

- Arduino UNO
- LCD 16x2 con interfaz I2C, dirección `0x27`
- LED rojo y resistencia de 220 Ω en el pin 13
- LED amarillo y resistencia de 220 Ω en el pin 12

El proyecto está preparado para ejecutarse en Wokwi sin Arduino IDE ni extensiones adicionales.

## Formato de entrada

Enviar una palabra por línea desde el Monitor Serie a 9600 baudios:

- 32 caracteres binarios, por ejemplo: `00000001100000011100100011000001`
- Hexadecimal: `0x` seguido de exactamente 8 dígitos, por ejemplo: `0x0181C8C1`

Formatos aceptados anteriormente por el proyecto original: **dependencia no disponible en este repositorio**. No existe documentación ni código del proyecto anterior. Si ese proyecto enviaba otra cosa (por ejemplo, mensajes CSV, prefijos distintos o varios campos en una línea), se debe definir y documentar primero ese formato serial. Mientras tanto, esta implementación usa exclusivamente una palabra por línea.

## Interpretación de la palabra

El código interpreta los campos habituales:

- Bits 1 a 8: Label. Se invierte el byte de 8 bits y se muestra en octal.
- Bits 12 a 29: campo de datos de 19 bits.
- Bits 30 a 31: SSM; se leen pero no se muestran.
- Bit 32: paridad. Se exige paridad impar.

El programa no intenta corregir ni aceptar palabras con paridad distinta de impar. Puede rechazar tanto el formato como la paridad.

## Ejecución en Wokwi desde el navegador

1. Abrir https://wokwi.com/projects en el navegador.
2. Crear un proyecto nuevo de Arduino UNO.
3. Subir o pegar `sketch.ino` como archivo principal.
4. Crear `diagram.json` o copiar su contenido en el editor visual del proyecto.
5. Pulsar **Start Simulation**.
6. Abrir el **Serial Monitor**.
7. Introducir una línea válida de `ejemplos-palabras.txt` y pulsar Enter.

No es necesario instalar Arduino IDE ni ninguna extensión.

> Si Wokwi no encuentra la librería I2C de la pantalla, agregar `LiquidCrystal I2C` como librería en el buscador de librerías del proyecto. El componente y la dirección están configurados para `0x27`.

## Comportamiento de los pines

- Cuando el bit transmitido es `1`: pin 13 en HIGH y pin 12 en LOW.
- Cuando el bit transmitido es `0`: pin 12 en HIGH y pin 13 en LOW.
- Los 32 bits se recorren desde el bit 32 hasta el bit 1, con 10 ms por bit.
- Al terminar, ambos pines quedan en LOW.

Las palabras deben cumplir paridad impar para activar la reproducción de bits. Una palabra con paridad incorrecta no cambia los pines a nivel válido.

## Ejemplos

Los ejemplos completos están en [`ejemplos-palabras.txt`](ejemplos-palabras.txt). Ejemplo válido:

```text
0x0181C8C1
```

Resultado esperado:

```text
Label octal=313
Valor=12345
Paridad=OK
```

Ejemplo de error de paridad:

```text
0x0181C8C0
```

El Monitor Serie informará `ERROR: paridad incorrecta` y la pantalla mostrará `Paridad incorrecta`.

## Organización del código

- `sketch.ino`: programa Arduino y funciones de lectura, validación, LCD y pines.
- `diagram.json`: circuito Wokwi para UNO, LCD y LEDs.
- `ejemplos-palabras.txt`: datos válidos e inválidos para pruebas.
