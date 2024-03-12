### esp32 helper board
This program makes the esp32 output voltages and enables control of the LED over serial. It is running at 115200 baud with the standard bit configuration.

Voltages are outputted as
```
adc1 3.1415\r\n
adc2 2.2123\r\n
```

ADC1 corresponds to GPIO1 and ADC2 to GPIO2

to control the led send

`rgb 126 255 0\n`


#### building
The project is build using platformio. I recommend installing into a virtual environment with pip (see the interface documentation).

`pip install platformio`

`pio run -t upload`

and it should be done