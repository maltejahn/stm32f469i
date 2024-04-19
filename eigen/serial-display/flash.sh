make clean
make
arm-none-eabi-objcopy -O binary serial-display.elf serial-display.bin
sudo st-flash write serial-display.bin 0x08000000
sudo st-flash reset

