export DEVKITPRO ?= /opt/devkitpro
export DEVKITARM ?= $(DEVKITPRO)/devkitARM
TARGET := PerroSanchez_DS
BUILD := build
SOURCES := source
INCLUDES := source
ARCH := -mthumb -mthumb-interwork
CFLAGS := -g -Wall -O2 -ffast-math $(ARCH) -fomit-frame-pointer -ffunction-sections -fdata-sections
CFLAGS += $(INCLUDE) -DARM9
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS := -g $(ARCH)
LDFLAGS := -specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS := -lnds9

include $(DEVKITARM)/ds_rules
