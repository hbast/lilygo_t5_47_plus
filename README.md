# LILYGO T5 4.7 Plus for ESPHome

External ESPHome component for the **LILYGO T5 4.7 Plus** E-Paper board with
ESP32-S3 (hardware versions 2.3 and 2.4).

The repository provides one component name for all supported functions:

- `display: platform: lilygo_t5_47_plus`
- `touchscreen: platform: lilygo_t5_47_plus`
- `sensor: platform: lilygo_t5_47_plus`

The component requires the Arduino framework and an ESP32-S3 with 8 MB octal
PSRAM. The display resolution is 960 x 540 pixels with 16 grayscale levels.

## Quick start

The package configures the board, flash, PSRAM, I2C bus and external component.
Only this repository needs to be referenced:

```yaml
esphome:
  name: lilygo-t5-47-plus
  friendly_name: LILYGO T5 4.7 Plus

packages:
  lilygo_t5_47_plus: github://hbast/lilygo_t5_47_plus/packages/lilygo_t5_47_plus.yaml@v1.0.0

logger:
  hardware_uart: USB_SERIAL_JTAG

display:
  - platform: lilygo_t5_47_plus
    id: epaper
    update_interval: 60s
    lambda: |-
      it.fill(COLOR_OFF);
      it.rectangle(20, 20, 920, 500, COLOR_ON);
```

See [`examples/`](examples/) for display, grayscale, touchscreen, battery and
deep-sleep configurations.

## Hardware configuration

[`packages/board.yaml`](packages/board.yaml) is the single source of truth for
the board:

- ESP32-S3, Arduino framework
- 16 MB QIO flash
- 8 MB octal PSRAM at 80 MHz
- GT911 I2C: SDA GPIO18, SCL GPIO17 at 400 kHz
- Battery ADC: GPIO14
- Wake/button input: GPIO21

Do not copy PlatformIO or SDK configuration blocks into device YAML files. Use
the package so board settings remain consistent with the component.

## Partial (non-flashing) refresh

By default every `component.update` performs a full refresh (`epd_clear()` +
grayscale draw), which flashes the whole panel. For frequently updating
dashboards you can enable pixel-diff partial refresh:

```yaml
display:
  - platform: lilygo_t5_47_plus
    id: epaper
    full_update_every: 15   # full clear every 15th refresh (default: 1 = always full)
```

With `full_update_every` > 1, the component keeps the previous frame and only
applies ink to pixels that changed (erase pass with `WHITE_ON_WHITE`, draw pass
with `BLACK_ON_WHITE`), so unchanged screen areas do not flicker at all.
Partial refreshes slowly accumulate ghosting artifacts; the periodic full
refresh cleans them up. You can also force a clean full refresh from an
automation (useful when switching display pages):

```yaml
on_...:
  - lambda: id(epaper).force_full_update();
  - component.update: epaper
```

## Local development

Python, PlatformIO and compiler toolchains are kept inside Docker:

```sh
./scripts/dev build
./scripts/dev version
./scripts/check-repository
./scripts/dev config tests/minimal.yaml
./scripts/dev compile tests/minimal.yaml
```

See [`DEVELOPMENT.md`](DEVELOPMENT.md) for the full workflow.

## Hardware support

- V2.4 is hardware-tested with display, correct black/white polarity, all 16
  grayscale levels, transformed touch coordinates and timed deep sleep. The
  battery sensor reports 0% correctly without a battery; measurement with a
  connected LiPo remains to be verified.
- V2.3 uses the same ESP32-S3 display pinout. Boards without the optional touch
  panel can omit the `touchscreen` section.
- The original ESP32-based T5 4.7 board is not supported by this component.

## Credits and license

The E-Paper driver is derived from
[`vroland/epdiy`](https://github.com/vroland/epdiy) and LilyGO's
[`LilyGo-EPD47`](https://github.com/Xinyuan-LilyGO/LilyGo-EPD47). Source files
retain their copyright and SPDX notices.

The repository history also contains contributions from the earlier community
implementations. These are acknowledged as provenance; they are not runtime or
installation dependencies.

See [`LICENSE`](LICENSE).
