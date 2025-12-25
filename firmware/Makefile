BUILD_DIR?=../build/colorlight_i5

include $(BUILD_DIR)/software/include/generated/variables.mak
include $(SOC_DIRECTORY)/software/common.mak
INCLUDES += -I$(CURDIR)/incs/aht10
INCLUDES += -I$(CURDIR)/incs/time_driver
INCLUDES += -I$(CURDIR)/incs/i2c_driver
INCLUDES += -I$(CURDIR)/incs/BH1750
INCLUDES += -I$(CURDIR)/incs/max3010x
INCLUDES += -I$(CURDIR)/incs/gfx
INCLUDES += -I$(CURDIR)/incs/ST7789
INCLUDES += -I$(CURDIR)/incs/TCS34725

OBJECTS   = crt0.o main.o incs/aht10/aht10.o incs/time_driver/time_driver.o incs/i2c_driver/i2c_driver.o incs/BH1750/bh1750.o incs/max3010x/max3010x.o incs/gfx/gfx.o incs/ST7789/ST7789.o incs/TCS34725/TCS34725.o
all: main.bin

# pull in dependency info for *existing* .o files
-include $(OBJECTS:.o=.d)

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
	chmod -x $@

main.elf: $(OBJECTS)
	$(CC) $(LDFLAGS) -T linker.ld -N -o $@ \
		$(OBJECTS) \
		$(PACKAGES:%=-L$(BUILD_DIR)/software/%) \
		-Wl,--whole-archive \
		-Wl,--gc-sections \
		-Wl,-Map,$@.map \
		$(LIBS:lib%=-l%)
	chmod -x $@

crt0.o: $(CPU_DIRECTORY)/crt0.S
	$(assemble)

%.o: %.c
	$(compile)

%.o: %.S
	$(assemble)

clean:
	$(RM) $(OBJECTS) main.elf main.bin .*~ *~

.PHONY: all clean
