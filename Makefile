# Makefile for ESP8266 projects
#
# MIT License
# Copyright (c) 2020 Holger Mueller

VERBOSE		= 0
FLAVOR		= release

# name for the target project
TARGET		= wiringESP
# name of the target library
TARGET_LIB	= lib$(TARGET).a

# Output directors to store intermediate compiled files
# relative to the project directory
BUILD_BASE	= build
LIB_BASE	= $(BUILD_BASE)

# Root of Espressif toolset (compiler, SDK)
ESP_ROOT	?= /opt/Espressif

# base directory for installing the library
PREFIX	?= $(ESP_ROOT)/local
LIBDIR	= ${PREFIX}/lib
INCDIR	= ${PREFIX}/include

# base directory for the compiler
XTENSA_TOOLS_ROOT ?= $(ESP_ROOT)/xtensa-lx106-elf/bin

# base directory of the ESP8266 SDK package, absolute
SDK_BASE	?= $(ESP_ROOT)/ESP8266_NONOS_SDK-3.0.4

# which modules (subdirectories) of the project to include in compiling
MODULES		= src
EXTRA_INCDIR	= include include/wiringESP

# libraries used in this project, mainly provided by the SDK
LIBS		=
EXTRA_LIBDIR	=

# compiler flags using during compilation of source files
ifeq ($(FLAVOR),debug)
    DEFINES	+= -DDEBUG_ON
    CFLAGS	+= -g
endif
DEFINES		+= -D__ets__ -DICACHE_FLASH -DSPI_FLASH_SIZE_MAP=$(SIZE_MAP)
CFLAGS		+= -Os -std=c99 -Wpointer-arith -Wundef -Werror -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals $(DEFINES)
CXXFLAGS	= $(subst -std=c99,-std=c++11,$(CFLAGS)) -fno-rtti -fno-exceptions

# linker flags used to generate the main object file
LDFLAGS		= -nostdlib -u call_user_start -Wl,-static -Wl,--gc-sections


####
#### no user configurable options below here
####

# set tools to use as compiler, librarian and linker
CC			:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
CXX			:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-g++
AR			:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-ar
LD			:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
OBJCOPY		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-objcopy
INSTALL		:= /usr/bin/install

# various paths from the SDK used in this project
SDK_LIBDIR	:= lib
SDK_LDDIR	:= ld
SDK_INCDIR	:= include

SRC_DIR		:= $(MODULES)
BUILD_DIR	:= $(addprefix $(BUILD_BASE)/,$(MODULES))

SDK_LIBDIR	:= $(addprefix $(SDK_BASE)/,$(SDK_LIBDIR))
SDK_INCDIR	:= $(addprefix -I$(SDK_BASE)/,$(SDK_INCDIR))

SRC			:= $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c) $(wildcard $(sdir)/*.cpp))
OBJ			:= $(patsubst %.c,$(BUILD_BASE)/%.o,$(SRC))
OBJ			:= $(patsubst %.cpp,$(BUILD_BASE)/%.o,$(OBJ))
DEP			:= $(patsubst %.c,$(BUILD_BASE)/%.d,$(SRC))
DEP			:= $(patsubst %.cpp,$(BUILD_BASE)/%.d,$(DEP))
LIBS		:= $(addprefix -l,$(LIBS))
APP_AR		:= $(addprefix $(BUILD_BASE)/,$(TARGET)_app.a)
TARGET_OUT	:= $(addprefix $(BUILD_BASE)/,$(TARGET).out)
OUTPUT_LIB	:= $(addprefix $(LIB_BASE)/,$(TARGET_LIB))

SRC_INCDIR		:= $(addprefix -I,$(SRC_DIR))
EXTRA_INCDIR	:= $(addprefix -I,$(EXTRA_INCDIR))
MODULE_INCDIR	:= $(addsuffix /include,$(SRC_INCDIR))

EXTRA_LIBDIR	:= $(addprefix -L,$(EXTRA_LIBDIR))

ifeq ("$(VERBOSE)", "1")
    Q :=
    #vecho := @true
    vecho := @echo
else
    Q := @
    vecho := @echo
endif

vpath %.c $(SRC_DIR)
vpath %.cpp $(SRC_DIR)

define compile-objects
$1/%.d: %.c
	$(vecho) "GEN $$@"
	$(Q) $(CC) $(SRC_INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CFLAGS) -MM $$< | sed 's;\(.*\)\.o[ :]*;$$(dir $$@)\1.o $$@ : ;g' > $$@
$1/%.d: %.cpp
	$(vecho) "GEN $$@"
	$(Q) $(CXX) $(SRC_INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CXXFLAGS) -MM $$< | sed 's;\(.*\)\.o[ :]*;$$(dir $$@)\1.o $$@ : ;g' > $$@
$1/%.o: %.c
	$(vecho) "CC $$<"
	$(Q) $(CC) $(SRC_INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CFLAGS) -c $$< -o $$@
$1/%.o: %.cpp
	$(vecho) "C+ $$<"
	$(Q) $(CXX) $(SRC_INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CXXFLAGS) -c $$< -o $$@
endef

.PHONY: all checkdirs clean clobber install uninstall

all: checkdirs $(DEP) $(OUTPUT_LIB)

checkdirs: $(BUILD_DIR) $(LIB_BASE)

$(BUILD_DIR):
	$(info CREATE $@)
	$(Q) mkdir -p $@

$(LIB_BASE):
	$(info CREATE $@)
	$(Q) mkdir -p $@

$(OUTPUT_LIB): $(OBJ)
	$(info AR $@)
	$(Q) $(AR) cru $@ $^

install: all
	$(Q) $(INSTALL) -d $(LIBDIR)
	$(INSTALL) $(OUTPUT_LIB) $(LIBDIR)
	$(Q) $(INSTALL) -d $(INCDIR)
	cp -a include/$(TARGET) $(INCDIR)

uninstall:
	rm -f $(LIBDIR)/$(TARGET_LIB)
	rm -rf $(INCDIR)/$(TARGET)

clean:
	$(Q) rm -rf $(BUILD_BASE) $(LIB_BASE)

# include deps only if exist (after first run)
ifeq ($(wildcard $(DEP)),$(DEP))
ifneq ($(filter clean,$(MAKECMDGOALS)),clean)
include $(DEP)
endif
endif

$(foreach bdir,$(BUILD_DIR),$(eval $(call compile-objects,$(bdir))))
