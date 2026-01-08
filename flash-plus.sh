#!/bin/bash
# Flash script for WT32-SC01 PLUS (ESP32-S3)

PORT=${1:-/dev/ttyACM0}
BUILD_DIR=./build
BOOT_APP0=/home/cav/.arduino15/packages/esp32/hardware/esp32/2.0.17/tools/partitions/boot_app0.bin

echo "Flashing WT32-SC01 PLUS on port $PORT..."
echo "Si la connexion échoue, maintenez le bouton RESET pendant 2 secondes..."

# ESP32-S3 uses USB-Serial/JTAG mode
python3 -m esptool \
  --chip esp32s3 \
  --port $PORT \
  --baud 460800 \
  --before default-reset \
  --after hard-reset \
  write-flash -z \
  --flash-mode dio \
  --flash-freq 80m \
  --flash-size 16MB \
  0x0000 $BUILD_DIR/fossa-plus.ino.bootloader.bin \
  0x8000 $BUILD_DIR/fossa-plus.ino.partitions.bin \
  0xe000 $BOOT_APP0 \
  0x10000 $BUILD_DIR/fossa-plus.ino.bin

echo ""
echo "✅ Flash terminé ! Votre WT32-SC01 PLUS devrait redémarrer automatiquement."
echo "Si ce n'est pas le cas, appuyez sur le bouton RESET."
