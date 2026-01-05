# High Resolution Mouse Scrolling
This Repo implements high resolution scrolling for Raspberry pi pico using the pico SDK pluggin and the Arduino IDE and TinyUSB stack

The device used AS5600 magnetic encoder with a 3d printed case.

The sensitivity can be set using the `sensitivity` variable. 

The code was copied over from https://github.com/krucho/HighResScroll and adapted to use the magnetic encoder.
The magnetic encoder is connected to the second I2C bus I2C1.
To adapt to I2C bus change Wire1 to Wire.

This project was the inspired from https://www.youtube.com/watch?v=tzqJ1rJURgs
