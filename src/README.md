# WiFiLed

With this project, you can control an WS2812 RGB LED stripe (up to 300 LEDs) via your browser.<br>
The following features are realized.

* **browser control**<br>
  The device provides a webpage, where you can control the LED stripe (including simple animations).<br>
  ![mainScreen](doc/mainScreen.png)
* **smooth on/off**<br>
  The LEDs will be turned on and off smoothly via a PT1 damping
* **rightness is dependent form sunrise & sunset**<br>
  You can define two different brightnesses, one for sunrise and one for sunset.<br>
  The brightness switches automatically when the sun rises or sets.<br>
  ![ledSetup](doc/ledSetup.png)
* **color animations**<br>
  You can define simple color animations<br>
  ![colorMode](doc/colorMode.png)
* **sunrise & sunset calculation**<br>
  You can configure NTP timing servers and your longitude and latitude, to calculate the sunrise and sunset.<br>
  ![timeSetup](doc/timeSetup.png)
* **optionally IR motion sensors**<br>
  Connect optionally two IR motion sensors to activate the LEDs by motion detection.<br>
  After a defined time the LEDs will be durned off.<br>
  ![globalSetup](doc/globalSetup.png)
* **optionally IR distance sensors**<br>
  Connect optionally an IR distance sensor to control on/oof and the brightness.<br>
  * fast moving across the distance sensor turns the LEDs on/off
  * static distance in front of the distance sensor changes the brightness

----
## Hardware
The following main devices are used in the schema:

1. **Wemos D1 mini**: ESP8266 CPU<br>
    doc: https://www.wemos.cc/en/latest/d1/d1_mini.html
1. **WS2812B**: single addressable LED-stripe<br>
   doc: [data sheet](/doc/WS2812B.pdf)
1. **AM312**: IR motion sensor (optional)<br>
   doc: [data sheet](/doc/AM312.pdf)
1. **GP2Y0A21YK0F**: IR distance sensor (optional)<br>
   doc: [data sheet](/doc/GP2Y0A21YK0F.pdf)

### Circuit Diagram
![schema](doc/schema.png)

### Net-Plan
![net plan](doc/NetPlan.png)

----
## Compile & Flash

1. open **WiFiLed.ino** in [Arduino IDE](https://www.arduino.cc/en/software)
1. select board **LOLIN(WEMOS) D1 R2 & mini**
1. install the following libraries:

    * [NeoPixelBus](https://github.com/Makuna/NeoPixelBus)
    * [WebSockets](https://www.arduinolibraries.info/libraries/web-sockets)
    * [WiFi](https://www.arduinolibraries.info/libraries/wi-fi)
1. connect device **Wemos D1 mini** via USB
1. configured the connected COM port in [Arduino IDE](https://www.arduino.cc/en/software) (menue: tools/port)
1. upload ESP8266 Sketch Data (menue: tools/ESP8266 Sketch Data Upload)
1. compile an link the code via [Arduino IDE](https://www.arduino.cc/en/software)
2. upload the code via [Arduino IDE](https://www.arduino.cc/en/software)

## Debug output
This project sends a lot of debug information via the serial interface. These data will be sent with **115200 baud**.<br>
You can control which information should be sent, when you change the `DEBUG_LEVEL`in file [DebugLevel.h](https://github.com/DaWaGit/WiFiLed/blob/main/DebugLevel.h)

https://github.com/DaWaGit/WiFiLed/blob/8eb0543011bb023a1f4afb5fbdf0c67800a0c655/DebugLevel.h#L4-L20

----
## Getting Started
When you turn on as first time, follow the next steps, to connect the device in your WiFi:

1. The blue onboard LED is blinking slowly to inform you, a Wifi-Access-Point was started.
1. Search and connect your mobile device with an WiFi-Access-Point named `WiFiLed-\<DeviceId\>`<br>
   | name     | value                |
   | -------- | -------------------- |
   | SSID     | WiFiLed-\<DeviceId\> |
   | Password | none                 |
   | Url      | http://192.168.4.1/  |

1. Open in your browser the page http://192.168.4.1/
1. In **WiFi Setup** add your WiFi SSID and password and press **Success**<br>
   ![wifiSetup](doc/wifiSetup.png)
1. The device starts new, but with a fast blinking onboard LED to inform you, the connection to your WiFi is trying
1. When the WiFi connection is successfully established, the onboard LED is static on.<br>

   * Search in your router the IP from device
   * open the IP in your browser to control the LED stripe
1. When the WiFi connection fails after 3 minutes, the device will restart, but im WiFi-Access-Point mode (slow blinking onboard LED)<br>
   Repeat all steps from begin

----
## Known Issues

* none
