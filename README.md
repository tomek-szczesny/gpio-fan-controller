# gpio-fan-controller
An universal fan control daemon for any single board computer.

gpio-fan-controller drives a defined GPIO with a PWM signal, that can be used to drive an external fan (through an npn transistor, for example).
It works with any single board computer, through libgpiod.

A simple algorithm monitors CPU temperature and drives a PWM output, so it tries to reach a target temperature (45 deg C by default). In practice, this can be seen as a turn-on threshold temperature. 

## Installation

First, install the prerequisites:
- g++
- make
- git
- libgpiod_dev, gpiod

Clone this repository:

`git clone https://github.com/tomek-szczesny/gpio-fan-controller.git`

Inspect the first 30 lines of `gpio-fan-controller.cpp`. Here, you must configure which GPIO you wish to use.

You can use `sudo gpioinfo` to find the correct gpiochip and line numbers.

Next:
```
make build
sudo make install
```

From now on, the daemon should be installed and working.
