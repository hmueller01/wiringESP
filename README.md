# About wiringESP
This is a partial implementation of the [Arduino](http://www.arduino.cc/)
[Wiring API](http://wiring.org.co/) for the
[ESP8266 SDK](https://github.com/espressif/ESP8266_NONOS_SDK).

### Requirements
* ESP8266 board, e.g. ESP12E (ebay)

### Installation and usage
* Install the cross compiler [esp-open-sdk](https://github.com/pfalcon/esp-open-sdk)
* Install the [ESP8266 SDK](https://github.com/espressif/ESP8266_NONOS_SDK)
* Add ```wiringESP.c``` and ```wiringESP.h``` to your make process.
* Use as documented by the [Wiring API reference](http://wiring.org.co/reference/)

### Notes
Not all Wiring API stuff is implemented. Basically things needed by 
[rc-switch](https://github.com/sui77/rc-switch) is done.