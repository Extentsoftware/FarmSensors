# Makefile for ATTINY85 Soil Moisture Sensor
# Compiler and tools
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
AVRDUDE = avrdude

# Target settings
MCU = attiny85
F_CPU = 8000000UL
TARGET = soil_moisture_sensor

# Compiler flags
CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Wall -Wextra -Os -std=c99
CFLAGS += -fshort-enums -fpack-struct -funsigned-char -funsigned-bitfields
CFLAGS += -fno-strict-aliasing -fno-jump-tables

# Linker flags
LDFLAGS = -mmcu=$(MCU) -Wl,--gc-sections

# Source files
SOURCES = attiny85_firmware.c
OBJECTS = $(SOURCES:.c=.o)

# AVRDUDE settings
AVRDUDE_PROGRAMMER = usbasp
AVRDUDE_PORT = usb
AVRDUDE_FLAGS = -p $(MCU) -c $(AVRDUDE_PROGRAMMER) -P $(AVRDUDE_PORT)

# Default target
all: $(TARGET).hex $(TARGET).lst

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files
$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

# Create Intel HEX file
$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

# Create listing file
$(TARGET).lst: $(TARGET).elf
	$(OBJDUMP) -h -S $< > $@

# Show size information
size: $(TARGET).elf
	$(SIZE) $<

# Program the microcontroller
program: $(TARGET).hex
	$(AVRDUDE) $(AVRDUDE_FLAGS) -U flash:w:$<:i

# Program with fuse settings
fuses:
	$(AVRDUDE) $(AVRDUDE_FLAGS) -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET).elf $(TARGET).hex $(TARGET).lst

# Install dependencies (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install gcc-avr avr-libc avrdude

# Help
help:
	@echo "Available targets:"
	@echo "  all        - Build the project (default)"
	@echo "  clean      - Remove build files"
	@echo "  program    - Program the ATTINY85"
	@echo "  fuses      - Set fuse bits"
	@echo "  size       - Show memory usage"
	@echo "  install-deps - Install required packages"
	@echo "  help       - Show this help"

.PHONY: all clean program fuses size install-deps help
