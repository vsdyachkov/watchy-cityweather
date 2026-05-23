#!/usr/bin/env bash
set -euo pipefail

VERSION="${1:-$(git describe --tags --always --dirty | sed 's/-dirty$//')}"
PIO_ENV="${PIO_ENV:-watchy}"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/.pio/build/$PIO_ENV"
OUT_DIR="$PROJECT_DIR/docs/firmware"
BOOT_APP0="${BOOT_APP0:-$HOME/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin}"

mkdir -p "$OUT_DIR"

CITYWEATHER_VERSION_OVERRIDE="$VERSION" pio run -e "$PIO_ENV"

rm -f "$OUT_DIR/cityweather-watchy-$VERSION.bin"
cp "$BUILD_DIR/bootloader.bin" "$OUT_DIR/cityweather-watchy-$VERSION-bootloader.bin"
cp "$BUILD_DIR/partitions.bin" "$OUT_DIR/cityweather-watchy-$VERSION-partitions.bin"
cp "$BOOT_APP0" "$OUT_DIR/cityweather-watchy-$VERSION-boot_app0.bin"
cp "$BUILD_DIR/firmware.bin" "$OUT_DIR/cityweather-watchy-$VERSION-firmware.bin"

cat > "$OUT_DIR/manifest.json" <<JSON
{
  "name": "CityWeather",
  "version": "$VERSION",
  "new_install_prompt_erase": true,
  "new_install_improv_wait_time": 0,
  "builds": [
    {
      "chipFamily": "ESP32",
      "improv": false,
      "parts": [
        {
          "path": "cityweather-watchy-$VERSION-bootloader.bin",
          "offset": 4096
        },
        {
          "path": "cityweather-watchy-$VERSION-partitions.bin",
          "offset": 32768
        },
        {
          "path": "cityweather-watchy-$VERSION-boot_app0.bin",
          "offset": 57344
        },
        {
          "path": "cityweather-watchy-$VERSION-firmware.bin",
          "offset": 65536
        }
      ]
    }
  ]
}
JSON

echo "Created split firmware files in $OUT_DIR"
