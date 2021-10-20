CROSS_COMPILE ?= arm-none-eabi-
CC := $(CROSS_COMPILE)gcc
AS := $(CROSS_COMPILE)as

CFLAGS = -fno-common -ffreestanding -O0 -gdwarf-2 -g3 -Wall -Werror \
		 -mcpu=cortex-m3 -mthumb -Wl,-Tlinker.ld,-Map=map.map -nostartfiles
	 
SRCS :=
SRCS += interrupt.c
SRCS += kernel.c
SRCS += clock.c
SRCS += systick.c
SRCS += test_functions.c
SRCS += debug_printf.c
SRCS += uart.c
	 
INCS :=
INCS += -I.
INCS += -I./CMSIS/Device/ST/STM32F1xx/Include
INCS += -I./CMSIS/Include

TARGET_NAME = myos

###################################
# End of configuration parameters #
###################################

OBJS = $(SRCS:.c=.o)

TARGET_BIN = $(addsuffix .bin,$(TARGET_NAME))
TARGET_ELF = $(addsuffix .elf,$(TARGET_NAME))
TARGET_LIST = $(addsuffix .list,$(TARGET_NAME))

all: $(TARGET_BIN)
	@echo Generating $(TARGET_BIN) and $(TARGET_LIST)
	@$(CROSS_COMPILE)objcopy -Obinary $(TARGET_ELF) $(TARGET_BIN)
	@$(CROSS_COMPILE)objdump -S $(TARGET_ELF) > $(TARGET_LIST)

$(TARGET_BIN): $(OBJS)
	@echo Linking object files for $(TARGET_ELF)
	@$(CC) $(CFLAGS) $(INCS) -o $(TARGET_ELF) $^
	
%.o : %.c
	@echo Compiling $^
	@$(CC) $(CFLAGS) $(INCS) -o $@ -c $^
	
list_sources:
	@echo Source files: $(SRCS)
	@echo Object files: $(OBJS)
	@echo Include folders: $(INCS)
	
clean:
	rm -f *.o *.elf *.bin *.list *.map
