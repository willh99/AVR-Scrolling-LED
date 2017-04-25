# AVR-Scrolling-LED
Scrolling, ASCII Capable 8x8 MAX7219 LED matrix

Project by William Horowitz
The George Washingnton University School of Engineering and Applied Science

Credit to https://embeddedthoughts.com/2016/04/19/scrolling-text-on-the-8x8-led-matrix-with-max7219-drivers/ for the source of the ascii conversion


This project was done using an ATmega16 microcontroller on an STK500 board. The ucontroller communicates with the MAX7219 LED array using the PORTB pins in SPI mode.The message and scrolling speed can be altered using a button. An interrupt checks for a button press (in my case a capacitive touch sensor was attached to PORTA). If a button press event is detected, a different timer is polled in order to determine the legth of time the button has been pressed. <1sec changes the message displayed, 1-2 seconds slows down the scrolling speed, 2 seconds speeds up the scrolling speed.
