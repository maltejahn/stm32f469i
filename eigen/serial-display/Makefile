#

OBJS = ../../demos/util/lcd.o ../../demos/util/hexdump.o ../../demos/util/console.o \
	   ../../demos/util/clock.o ../../demos/util/sdram.o ../../demos/util/retarget.o ../../demos/util/sbrk.o \
		../../demos/util/touch.o ../../demos/util/i2c.o

BINARY = serial-display

DEVICE = STM32F469HI

#LDSCRIPT = ../stm32f469i-discovery.ld
#LDSCRIPT = ./stm32f4-disco.ld

CFLAGS = -I../../simple-graphics/include
LDFLAGS = -L../../simple-graphics/lib
LDLIBS = -lgfx -lm

include ../../Makefile.include

