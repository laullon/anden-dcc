# anden-dcc

Central DCC para diorama H0 con Raspberry Pi Pico W y Deek-Robot L298P.

## Estado de la versión 0.2.2

Compilado con Arduino-Pico 6.3.1 y cargado por USB en la Pico W. Arranque
comprobado por diagnóstico serie. Conexión a router, panel y OTA pendientes de
prueba desde un cliente Wi-Fi. Incluye configuración persistente de red, reconexión
automática, punto de acceso Wi-Fi,
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
- Abrir http://192.168.42.1 (IP fija del punto de acceso).
- Introducir el SSID y la contraseña de la red de 2,4 GHz en el formulario del panel.
- Los datos se guardan en LittleFS, dentro de la Pico, sin incluirlos en el código ni Git.
- Una contraseña vacía permite conectarse a una red abierta.
- La Pico obtiene una IP por DHCP; el panel muestra el enlace al conectarse.
- En la red doméstica también se anuncia `http://anden-dcc.local` mediante mDNS
  (su disponibilidad depende del cliente y la red).
- El punto de acceso abierto `anden-dcc` sigue disponible como recuperación;
  al conectar al router puede cambiar de canal y requerir reconectar el cliente.
- Si falla la conexión, reintenta cada 30 segundos. Se pueden corregir los datos
  desde el punto de acceso o borrarlos con «Olvidar red».
- Actualización: `/update`, sin usuario ni contraseña.
- Para OTA usar el `.bin` de una compilación compatible, no el `.uf2`.

Conservar acceso al conector USB y BOOTSEL para recuperación.
Antes de implementar DCC, incorporar apagado físico por defecto y deshabilitación
del puente durante actualizaciones y reinicios. No basta con detener una señal GPIO.

## Alimentación prevista

USB-C PD a 15 V para potencia, MP1584 a 5 V para lógica y Pico.
El esquema de alimentación, aislamiento de alimentación USB, adaptación ADC y
protección de corriente está pendiente de verificar; este código no autoriza su conexión.

## Compilación reproducible

Core: `rp2040:rp2040@6.3.1`.

```sh
arduino-cli compile --fqbn rp2040:rp2040:rpipicow:flash=2097152_1048576 --output-dir build firmware/anden-dcc
```

Primera carga realizada con `picotool load -x build/anden-dcc.ino.uf2`.
Diagnóstico serie USB a 115200: enviar `s` para consultar versión, estado DCC e IP.

El diagnóstico USB incluye el estado del almacenamiento y de la configuración,
sin revelar la contraseña. La memoria se inicializa automáticamente solo si su
primer bloque está borrado; un fallo de montaje no fuerza un formateo destructivo.

## LED integrado

- Parpadeo de un segundo (500 ms encendido y 500 ms apagado): sin conexión al router.
- Encendido fijo: conectado a la red Wi-Fi configurada.
- Si pierde la conexión, vuelve a parpadear. Conectar un móvil al punto de acceso
  no enciende el LED fijo; representa la conexión de la Pico al router.

El parpadeo usa `millis()` y no bloquea la atención del panel.

## Corrección 0.2.2

Las llamadas de inicio y cierre de SimpleMDNS se realizan bajo `LWIPMutex`,
como exige el acceso a las API raw de lwIP en Arduino-Pico 6.3.1. Sin ese
bloqueo, el anuncio del nombre local podía disparar una aserción al conectar
al router y detener el firmware con el LED fijo. Se espera también una IP
asignada antes de indicar conexión y arrancar mDNS.

Compilación verificada. La prueba de conexión real y el despliegue de esta
corrección están pendientes de recuperar la placa mediante BOOTSEL.
