# anden-dcc

Central DCC para diorama H0 con Raspberry Pi Pico W y Deek-Robot L298P.

## Estado de la versión 0.1.0

Código inicial sin compilar ni probar en hardware. Incluye punto de acceso Wi-Fi,
página de estado y actualización web sin autenticación. No implementa DCC,
protección de corriente ni salidas para el shield. Probar únicamente la Pico por USB.

## Compilación y primera carga

Usar el core Arduino-Pico de Earle Philhower, placa Raspberry Pi Pico W.
URL del índice del core:
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json

Abrir firmware/anden-dcc/anden-dcc.ino. Seleccionar Flash Size de 2 MB
con 1 MB para sketch y 1 MB para filesystem (LittleFS), necesario para OTA.
La compilación debe caber tanto en el espacio de aplicación como en el de staging.
Generar el UF2 para la primera carga USB mediante BOOTSEL.

## Uso previsto

- Red: `anden-dcc`
- Red Wi-Fi abierta, sin contraseña.
- Abrir la IP del punto de acceso (normalmente http://192.168.42.1;
  confirmar por el puerto serie o la puerta de enlace de la conexión Wi-Fi).
- Actualización: `/update`, sin usuario ni contraseña.
- Para OTA usar el `.bin` de una compilación compatible, no el `.uf2`.

Conservar acceso al conector USB y BOOTSEL para recuperación.
Antes de implementar DCC, incorporar apagado físico por defecto y deshabilitación
del puente durante actualizaciones y reinicios. No basta con detener una señal GPIO.

## Alimentación prevista

USB-C PD a 15 V para potencia, MP1584 a 5 V para lógica y Pico.
El esquema de alimentación, aislamiento de alimentación USB, adaptación ADC y
protección de corriente está pendiente de verificar; este código no autoriza su conexión.
