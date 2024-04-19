make clean
make
arm-none-eabi-objcopy -O binary usart_irq_own.elf usart_irq_own.bin
sudo st-flash write usart_irq_own.bin 0x08000000
sudo st-flash reset

