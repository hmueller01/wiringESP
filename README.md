# About wiringESP
This is a partial implementation of the [Arduino](http://www.arduino.cc/)
[Wiring API](http://wiring.org.co/) for the
[ESP8266 SDK](https://github.com/espressif/ESP8266_NONOS_SDK).

### Requirements
* ESP8266 board, e.g. ESP12E (ebay)

### Prerequisites
* Install the cross compiler [esp-open-sdk](https://github.com/pfalcon/esp-open-sdk)
* Install the [ESP8266 SDK](https://github.com/espressif/ESP8266_NONOS_SDK)

### Installation and usage
_wiringESP_ can be compiled and installed as a local library
```bash
git clone --recursive https://github.com/hmueller01/wiringESP
cd wiringESP
#clean
make clean
#make
make ESP_ROOT=/opt/Espressif SDK_BASE=/opt/Espressif/ESP8266_NONOS_SDK
#installing
make PREFIX=/opt/Espressif/local install
```

Add `/opt/Espressif/local/lib` and `/opt/Espressif/local/include/` to your projects compile path. Use like this:
```c
#include "wiringESP/wiringESP.h"

...

pinMode(12, OUTPUT); // set GPIO pin 12 to output
digitalWrite(12, HIGH); // switch pin 12 high/on
```

For further usage see also [Wiring API reference](http://wiring.org.co/reference/) documention.

### Notes
Not all Wiring API stuff is implemented. Basically things needed by
[rc-switch](https://github.com/sui77/rc-switch) is done.
