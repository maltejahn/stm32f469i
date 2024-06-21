Status: 
- build and run
- no ESP till now
- lets say.. 5% overall

Trying to build a ESP32 powered WiFi Display with the STM32F469I-Disco Board based on ChuckM examples

Aim:
- porting to VSCode
- Using USART to comunicate between ESP and STM
- Getting USART6 in Interrupt mode to work -> main Problem till now
- ESP should get its data from a MQTT Broker

-- Original description

-> Changing libopencm3 to a more recent version


Usage:

Initialize submodules after git clone
    git submodule update --init --recursive
Use F1 + Select Configuration inside VSCODE to select one of the targets I`m playing around



STM32F469I-Discovery Code
-------------------------

I've got one of these boards from ST Micro and I have been
programming it with libopencm3 (sorry STCube). These are the
examples I've come up with so far.

I thought about suggesting them for the [libopencm3-examples][]
repo but there are already a bunch of examples there and these
are nominally redundant. That might change if I get an SDRAM and
Display example set up however.

[libopencm3-examples]: https://github.com/libopencm3/libopencm3-examples/
