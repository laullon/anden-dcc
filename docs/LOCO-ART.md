# Locomotive artwork

Three original, stylized SVG illustrations extend the existing flat palette.
The UI uses generic Electric / Diesel / Steam labels and no painted model
numbers (0.6.2). Prototype references below describe the inspiration only.
They are visual bases for electric, diesel and steam cards, not scale drawings
or claims that the decoder identifies a particular model.

| Saved type | Visual basis | Source asset |
| --- | --- | --- |
| `electric` | RENFE 269, green/yellow, roof pantographs and four axles | [electric.svg](../web/locos/electric.svg) |
| `diesel` | Original RENFE 333, green/yellow, twin cabs, grilles and six axles | [diesel.svg](../web/locos/diesel.svg) |
| `steam` | Santa Fe 151 / 5001, tender, five coupled driving axles and rods | [steam.svg](../web/locos/steam.svg) |

Choose the type in the loco editor. The preview changes locally, then Save stores
the choice with the name and address on the ESP. Existing cards default to
electric. Selecting a type does not send speed/function commands or reset F0.
The card shows a white headlight and red tail light when track output is active,
F0 is on and the finder is idle. Direction swaps the colors. The editor preview
lights both ends for illustration.

## Editing / embedding

Edit the three assets under `web/locos/`, then run:

```sh
python3 firmware/esp13-ota/embed.py
```

The script refreshes the marked template block in `firmware/esp13-ota/web/index.html` and the
embedded `SetupPage.cpp` (with declarations in `SetupPage.h`). CI checks both
for freshness. Each SVG is embedded once and cloned in the browser; it is not duplicated in firmware for every loco.
No image downloads, external fonts or raster assets are needed at runtime.

SVGs are compressed with the complete page and included in the ESP application.
The [PNG artwork preview](../assets/loco-types.png) uses simulated controller data.

## Visual references

References were used to observe the real/model silhouettes. No reference photos
are included in the application or repository; the SVGs are original artwork
under the repository's MIT license.

- [269 green/yellow Arnold model reference](https://www.gaugemasterretail.com/arnold-hn2608s-renfe-269-electric-locomotive-green-and-yellow-iv.html)
- [Original 333 green/yellow Roco model reference](https://www.eltallerdelmodelista.com/en/railway-modelling/scale-1-87-h0/locomotives/spain/36017-diesel-locomotive-series-333-renfe-roco-73702.html)
- [Santa Fe 151F-3101, Museu del Ferrocarril de Catalunya](https://museudelferrocarril.org/es/colecciones/coleccion.asp?cat=%27Locomotoras+de+vapor%27&id=25)
