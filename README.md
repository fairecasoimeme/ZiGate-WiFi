ZiGate-WiFi: Wifi-Serial Bridge based on ESP-LINK (Jeelab)
========================================

ZiGate-WiFi is based on powerfull ESP-LINK, you can follow and contribute to this project with following [esp-link github](https://github.com/jeelabs/esp-link)

TCP 9999 <--> ZiGate-WIFI <--> ZiGate radio

To get informations to install, config or other thingss, you can go to [WIKI](https://zigate.fr/Wiki_Categories/firmware-v2-xx/) from ZiGate Website 

Getting Started
---------------

To get started you need to:
 1. prepare your ZiGate-WiFi module for serial flashing (Push button when you plug the module)
 2. download the latest ZiGate-WiFi release image (you can build your own later)
 3. flash the firmware
 4. connect to ZIGATE-XXXX SSID (default password is adminXXXX)
 5. go to your favorite browser with [http://192.168.4.1](http://192.168.4.1)

 
### Quick links

In this document: [getting-started](#getting-started), [serial-bridge](#serial-bridge), [contact](#contact).

Separate documents:
- [hardware configuration](FLASHING.md), [serial flashing](FLASHING.md#initial-serial-flashing)
- [wifi configuration](WIFI-CONFIG.md)
- [troubleshooting](TROUBLESHOOTING.md), [LED indicators](TROUBLESHOOTING.md#led-indicators)
- [MQTT and outbound REST requests](RESTMQTT.md)
- [building esp-link](BUILDING.md), [over-the-air flashing](BUILDING.md#updating-the-firmware-over-the-air)
- [flash layout](FLASH.md)

 
### Serial bridge

In order to connect through the ZiGate-WiFi to ZiGate-Radio use port 9999. For example,
on linux you can use `nc esp-hostname 9999` or `telnet esp-hostname 9999`.

The connections on port 9999 and 2323 have a 5 minute inactivity timeout. This is standard with
Espressif's SDK and esp-link does not change it. The reason is that due to memory limitations only a
few connections can be open (4 per port) and it's easy for connections to get "lost" staying open
forever, for example, due to wifi disconnects. That could easily make it impossible to connect to
esp-link due to connection exhaustion. Something smarter is most likely possible...

Note that multiple connections to port 9999 and 2323 can be made simultaneously. Esp-link will
intermix characters received on all these connections onto the serial TX and it will
broadcast incoming characters from the serial RX to all connections. Use with caution!

If you are using esp-link to connect to the console of a linux system, such as an rPi, you
will most likely see what you typed being echoed twice. If you are on a linux system use
telnet and issue a `mode char` command (in telnet, hit the escape char `^]` and type `mode
char` at the prompt). If you are using putty on Windows, open the connection settings and
in the terminal settings set both `local echo` and `local line editing` to `off`.

### Features

- transparent bridge between ZiGate-WiFi and serial ZiGate-Radio
- debug ZiGate protocol with integrate console
- outbound REST HTTP requests from the attached micro-controller to the internet
- MQTT client pub/sub from the attached micro-controller to the internet
- upgrade ZiGate-WiFi over the air (OTA)

### Install

comming soon. Also you can go to [esp-link github](https://github.com/jeelabs/esp-link) to get more informations.

Contact
-------
If you find problems with ZiGate-WiFi, please create a github issue. 


### Versions

## v2.00
2019-05-07 -- Initial version 


