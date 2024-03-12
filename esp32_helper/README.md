### esp32 helper board
This program makes the esp32 output voltages and enables control of the LED over serial. It is running at 115200 baud with the standard bit configuration. It uses ESP-IDF with platformio.

Voltages are outputted as
```
adc1 3.1415\r\n
adc2 2.2123\r\n
```

ADC1 corresponds to GPIO1 and ADC2 to GPIO2

to control the led send

`rgb 126 255 0\n`


#### building
The project is built using platformio. I recommend installing it into a virtual environment with pip (see the interface readme).

`pip install platformio`


Then uploading should be as simple as

`pio run -t upload`

Platformio automatically finds the right port and enters the bootloader.