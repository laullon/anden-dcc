# anden-dcc

**Arduino Uno + motor shield L298P + shield Wi-Fi ESP13.**

> **La cláusula del humo mágico:** úsalo bajo tu propio criterio y riesgo.
> Se ofrece tal cual y sin garantía. Si tu Uno, puente H, decodificador o locomotora
> liberan humo mágico, te quedas con todas las piezas. El autor no se responsabiliza
> del humo, del olor ni de justificar el siguiente presupuesto ferroviario.

---

Una pequeña central para un diorama H0: luces, sonido y funciones desde el navegador.
El Uno genera DCC, el motor shield alimenta las vías y el ESP13 ofrece Wi-Fi,
panel web, almacenamiento y OTA. **ESP 0.7.0 / Uno 0.7.0.** El [mando con encoder](firmware/uno-stack-test/CAB.md)
opcional permite movimientos cortos y muestra la dirección DCC y el paso de velocidad.

- Hasta ocho locomotoras, direcciones cortas 1–127, con nombre e ilustración.
- F0–F28 con un único botón por función; F0 siempre aparece en cada locomotora.
- Puedes preparar funciones y sentido de marcha con la vía apagada.
- Abre **Explore functions**, prueba una función, ponle nombre y fíjala con
  **Pin favorite**. Hasta ocho favoritas por locomotora siguen visibles al plegar.
- Encendido/apagado explícito, con un control compacto visible al desplazarte.
- Buscador de dirección mediante preguntas sobre las luces; sin escribir CV.
- Temas claro/oscuro/sistema; Wi-Fi y actualizaciones dentro de Setup.

Los nombres, direcciones y favoritas sobreviven a un reinicio y a OTA normal.
Las funciones activas arrancan apagadas. Los botones son interruptores, no
pulsadores momentáneos. El significado de cada función depende del decodificador.

## Montaje y uso

Apila un Uno R3 de 5 V / 16 MHz, un Deek-Robot L298P con conexiones tipo Rev3 y un
DOIT ESP13. Mantén instalado el CD4077. Abre y verifica el enlace **VIN CONNECT**
del motor shield para separar sus 15 V de la entrada VIN del Uno. Alimenta la
lógica por USB y el motor shield mediante una fuente USB-C PD verificada a 15 V.
A+ y A− alimentan las dos vías en paralelo; ninguna vía es GND.

Para cargar el Uno: fuente del motor y vía desconectadas, USB conectado y ambos
interruptores del ESP13 en OFF. Para uso normal, ambos en ON. El ESP necesita una
primera carga serie; después se actualiza por OTA. OTA no actualiza el Uno.

Conecta a la Wi-Fi abierta **anden-esp13**, abre **http://192.168.13.1/** y configura
tu red de 2,4 GHz dentro de Setup. Vuelve a tu red y abre
**http://anden-esp13.local/** o la IP del ESP13 que aparece en **Your Wi-Fi**.
El panel está en inglés.

Edita las direcciones conocidas con la vía apagada. Si no conoces la dirección,
coloca una sola locomotora en la vía de prueba, enciende la salida y abre
**Find address**. Apaga la vía antes de guardar el resultado en una ficha.

Para actualizar el ESP13, termina la búsqueda, apaga las salidas y elige el
archivo `.bin` en **Setup → ESP13 firmware**. Recarga el panel tras el reinicio.

La protección por corriente aún no está calibrada. El sistema apaga la salida
al perder la conexión de control y no la reactiva automáticamente. Mantén la
alimentación del motor desconectada durante cargas y diagnósticos.

[Guía completa y compilación](README.md) · [Cableado y BOM](docs/hardware/README.md) ·
[API](docs/API.md) · [Validación](docs/VALIDATION.md)

## Licencia

[MIT](LICENSE). Úsalo, modifícalo, compártelo o véndelo. El humo de repuesto va por tu cuenta.
