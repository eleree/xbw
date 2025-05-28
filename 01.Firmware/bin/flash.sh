#!/bin/bash

# Check if IDF_PATH is set
if [ -z "$IDF_PATH" ]; then
  echo "Error: IDF_PATH environment variable is not set."
  exit 1
fi

# Construct the path to esptool.py
ESPTOOL_PATH="$IDF_PATH/components/esptool_py/esptool/esptool.py"

# Check if esptool.py exists
if [ ! -f "$ESPTOOL_PATH" ]; then
  echo "Error: esptool.py not found at $ESPTOOL_PATH"
  exit 1
fi

# Original arguments
ARGS="--chip esp32 --port /dev/ttyUSB0 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x1000 /home/esp32/github/xbw/01.Firmware/bin/bootloader/bootloader.bin 0xf000 /home/esp32/github/xbw/01.Firmware/bin/phy_init_data.bin 0x10000 /home/esp32/github/xbw/01.Firmware/bin/nesemu.bin 0x8000 /home/esp32/github/xbw/01.Firmware/bin/partitions.bin"

# Execute esptool.py
python "$ESPTOOL_PATH" $ARGS
