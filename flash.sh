#!/bin/sh
esptool.py --port /dev/ttyUSB0 --baud 921600 write_flash 0x00000 /home/esp8266/esp-open-sdk/ESP8266_NONOS_SDK-2.1.0/bin/boot_v1.2.bin 0x1000 firmware/user1.bin 0x3FE000 /home/esp8266/esp-open-sdk/ESP8266_NONOS_SDK-2.1.0/bin/blank.bin 0x3FC000 /home/esp8266/esp-open-sdk/ESP8266_NONOS_SDK-2.1.0/bin/esp_init_data_default.bin
