PROJECT = microphone
BUILD_DIR = bin

SHARED_DIR = 
CFILES = main.c usb_descriptors.c
CFILES += 
AFILES +=
LDLIBS+= -lm

# TODO - you will need to edit these two lines!
DEVICE=STM32F411CE
#OOCD_FILE = board/stm32f4discovery.cfg
OOCD_INTERFACE = stlink-v2
OOCD_TARGET = stm32f4x



# You shouldn't have to edit anything below here.
VPATH += $(SHARED_DIR)
INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))
OPENCM3_DIR=../libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
include ../rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk

