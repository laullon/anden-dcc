<p align="center"><img src="assets/banner.svg" alt="anden-dcc — control de trenes para un diorama" width="100%"></p>

# anden-dcc

## Garantía de humo mágico

**Úsalo bajo tu propio criterio y riesgo.** Es un proyecto de aficionado,
ofrecido tal cual y sin garantía. Si libera el humo mágico de tu Pico, puente H,
decóder o locomotora favorita, enhorabuena: has descubierto el efecto especial
menos útil del modelismo ferroviario. Los autores no se hacen responsables del
hardware dañado, las tardes descarriladas ni del humo que se niegue a volver al
chip. El sentido común y los electrones de repuesto los pones tú.

Una pequeña central para un diorama H0, con **Raspberry Pi Pico W**, control web,
Wi-Fi y OTA. La interfaz arranca en inglés; **ES** cambia al español y recuerda
la preferencia en ese navegador.

**Versión 0.4.0:** F0/luces y direcciones cortas 1–127. La señal DCC de 3,3 V en
GP2 ya está medida. **Todavía no se habilita la potencia ni el movimiento**.
El circuito de potencia está documentado como propuesta, pendiente de montaje,
pruebas y firmware de protección.

1. Conecta a `anden-dcc` y abre `http://192.168.42.1`.
2. Abre **Configuración**, introduce tu Wi-Fi de 2,4 GHz y guarda.
3. Vuelve a tu red y abre `http://anden-dcc.local` o la IP de la Pico.
4. Selecciona la dirección y usa los botones de luces F0.
5. Los archivos `.bin` se instalan en **Configuración → Actualizar firmware**.

Configuración está cerrada por defecto. El LED queda fijo conectado, parpadea
sin conexión y hace un pulso por peticiones de usuario; el sondeo automático de
estado no provoca ese pulso.

Para medir: **D0 → GP2/pin físico 4**, **GND → pin físico 3**. No conectes el
analizador directamente a una vía alimentada. Dirección 3: F0 apagado `03 80 83`;
encendido `03 90 93`.

- [README y compilación](README.md)
- [Cableado y comprobaciones](docs/hardware/README.md)
- [Materiales](docs/hardware/BOM.md)
- [Analizador](DCC-ANALYZER.md)
- [API](docs/API.md)

La documentación técnica está en inglés. Los datos Wi-Fi se guardan en la Pico,
no en el código. La red de configuración y OTA siguen abiertos por decisión del
proyecto para este prototipo local.

## Licencia

[Licencia MIT](LICENSE). Úsalo, modifícalo, compártelo, véndelo o monta tu propio
imperio ferroviario en miniatura. Conserva el aviso de licencia en las copias.

Las condiciones formales de garantía y responsabilidad están en la
[licencia MIT](LICENSE). Esta explicación con humo no añade restricciones a la
licencia.
