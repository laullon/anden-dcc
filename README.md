# anden-dcc

Central DCC para diorama H0 con Raspberry Pi Pico W y Deek-Robot L298P.

## Estado de la versión 0.3.0

Compilado con Arduino-Pico 6.3.1 y desplegado por OTA. Incluye Wi-Fi persistente,
panel web, mDNS, OTA y un primer generador DCC de nivel lógico en GP2 con control
F0 para direcciones cortas. No implementa velocidad, potencia de vías ni protección
de corriente. Ver [DCC-ANALYZER.md](DCC-ANALYZER.md) para el cableado del analizador,
paquetes esperados y validación. Las primeras versiones se cargaron por USB;
las versiones actuales pueden cargarse por OTA.

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
- La Pico obtiene una IP por DHCP; se puede consultar por USB o en el router.
- Abrir esa IP desde un equipo conectado a la misma red.
- Abrir `http://anden-dcc.local` desde la misma red (mDNS/Bonjour).
- mDNS arranca un segundo después de obtener IP y apagar el punto de acceso.
  Se detiene antes de cambiar de red y se reinicia al reconectar.
- La IP sigue funcionando aunque el cliente no resuelva nombres mDNS.
- El punto de acceso abierto `anden-dcc` se apaga cuando hay conexión al router.
  Si pierde la conexión, vuelve a habilitarlo al reintentar (hasta 30 segundos).
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
- Encendido fijo: conectado a la red Wi-Fi configurada, sin actividad web.
- Pulso apagado de 120 ms al abrir páginas, enviar acciones o acceder a OTA.
  Si llegan varias peticiones seguidas, el pulso se prolonga hasta 120 ms
  después de la última. Las consultas GET a `/api/status`, con o sin parámetros,
  se excluyen para que el refresco automático no haga parpadear el LED.
- Si pierde la conexión, vuelve a parpadear. Conectar un móvil al punto de acceso
  no enciende el LED fijo; representa la conexión de la Pico al router.

El parpadeo usa `millis()` y no bloquea la atención del panel.

## Corrección de acceso por red (0.2.3)

La versión con AP y STA simultáneos y mDNS llegaba a obtener una IP, pero
no respondía por HTTP. Añadir bloqueo explícito a mDNS en 0.2.2 no resolvió
el acceso. En 0.2.3 se ha retirado mDNS y se desconecta el AP tras obtener
una IP del router. No se ha aislado cuál de esos dos cambios elimina el fallo.
El acceso HTTP por IP funciona con esta configuración y la conexión guardada
se ha conservado durante las cargas USB.

Se mantiene la comprobación de dirección DHCP antes de indicar conexión.

## mDNS y OTA verificados (0.2.4)

mDNS se inicia únicamente en modo STA, tras apagar el AP de configuración y
obtener una IP por DHCP. La operación completa de inicio y cierre de SimpleMDNS
usa `LWIPMutex` para proteger el recorrido de interfaces. Se anuncia HTTP en el
puerto 80. El panel enlaza tanto a la IP como a `http://anden-dcc.local` y
`/api/status` informa del estado de mDNS.

Validación: compilado con Arduino-Pico 6.3.1; cargado por OTA desde 0.2.3 con
respuesta «Update Success»; 0.2.4 respondió después del reinicio por IP y nombre
mDNS, conservando la configuración Wi-Fi. Página principal y formulario OTA
accesibles por el nombre local. No se ha ensayado cortar la conexión del router.

Durante la validación hubo fallos transitorios de resolución y una muestra de
estado desconectado durante el arranque/reconexión. Después, Bonjour descubrió
el registro A y el servicio HTTP en ambas interfaces del Mac; una consulta mDNS
UDP directa recibió respuesta de la Pico con su IP correcta, y la página,
formulario OTA y API respondieron por el nombre local. Algunos intentos con
límite de cinco segundos agotaron la espera de resolución. Se conserva el acceso
por IP como alternativa; no se ha establecido la causa de esa latencia del cliente.

## Prueba OTA sin USB (0.2.5)

Versión 0.2.5 equivalente funcionalmente a 0.2.4; cambia el identificador para
comprobar el despliegue. Compilada con Arduino-Pico 6.3.1 y enviada mediante
POST multipart a `/update` por la red doméstica, sin utilizar USB. El servidor
respondió «Update Success» y, tras reiniciar, `/api/status` confirmó 0.2.5,
conexión Wi-Fi, mDNS y almacenamiento disponibles. Conservó la configuración.

## Actividad web en el LED (0.2.6)

Un hook del servidor marca actividad para todas las rutas, incluidas las de OTA.
El LED se apaga inmediatamente al recibir la petición y recupera su indicación
normal con un temporizador basado en `millis()`, sin introducir esperas. Mientras
se transmite un archivo OTA el bucle puede estar ocupado con la carga y el pulso
puede durar más de 120 ms. Sin Wi-Fi se mantiene el parpadeo de conexión existente.

Compilado y desplegado por OTA. Verificados el identificador de versión, el panel
y el formulario de actualización mediante peticiones HTTP. La observación física
del LED queda a cargo del usuario, ya que la Pico está conectada por red.

## Actividad sin parpadeo por sondeo (0.2.7)

El hook ignora GET `/api/status` (también con query string). El panel puede
seguir consultando el estado cada tres segundos sin alterar el LED. Las demás
peticiones mantienen el pulso de actividad; no se introduce un temporizador que
haga parpadear el LED por mantener abierta una página.

## Verificación física mediante Logic 2 MCP

Logic 2 2.4.48 detectó el analizador como un dispositivo físico Logic. Se
capturaron 100 ms a 8 MS/s en D0 para F0 apagado y encendido: 14 paquetes
completos por captura (`03 80 83` y `03 90 93`), sin errores XOR ni tiempos
inválidos. Semibits medidos de 58.000–58.125 us y 100.000–100.125 us.
Las capturas se exportaron desde MCP y se analizaron con
`tools/decode_dcc_csv.py`; no se utilizó simulación en esta medición.
