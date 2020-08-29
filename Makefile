# NOTE: Customize the following to your environment!

AVR_TOOLS_DIR = /usr
AVRDUDE_CONF=/usr/share/arduino/hardware/tools/avrdude.conf
ARDUINO_DIR=/home/smbaker/projects/pi/arduino-build/arduino
ALTERNATE_CORE_PATH=/home/smbaker/projects/pi/arduino-build/ATTinyCore/avr
ARDUINO_MK_FILE=/usr/share/arduino/Arduino.mk

ISP_PROG=usbasp

ALTERNATE_CORE = ATTinyCore
BOARD_TAG = attinyx5
BOARD_SUB = 85
F_CPU = 8000000L
ISP_LOCK_FUSE_PRE = 0xFF
ISP_LOCK_FUSE_POST = 0xFF
ISP_HIGH_FUSE = 0xDA
ISP_LOW_FUSE = 0xE2
ISP_EXT_FUSE = 0xFF

include $(ARDUINO_MK_FILE)
