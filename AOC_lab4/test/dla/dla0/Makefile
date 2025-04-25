ELF_NAME := main.elf

export CC ?= gcc
export CXX ?= g++

PROJECTBASE = $(PWD)
override PROJECTBASE    := $(abspath $(PROJECTBASE))
TOP_DIR = $(PROJECTBASE)

OBJ_DIR := obj

HW_DIR := $(TOP_DIR)/../../../hardware
SRC_DIR := $(TOP_DIR)/../../../src
INCLUDE_DIR := $(TOP_DIR)/../../../include

#######################################
# CFLAGS
#######################################
CFLAGS :=
LDFLAGS := -pthread -L$(HW_DIR) $(addprefix -l:,$(notdir $(wildcard $(HW_DIR)/*.a)))

# macros for gcc
# AS defines
AS_DEFS =
# C defines
C_DEFS =

QUIET = @

ifeq ($(QUIET),0)
	QUIET =
endif

PERLOG :=
ifeq ($(DLA_INFO),1)
	PERLOG += "======== DLA_INFO ========\n"
	C_DEFS += -DDLA_INFO
endif
ifeq ($(DEBUG),1)
	PERLOG += "======== DEBUG ========\n"
	C_DEFS += -DDEBUG
endif

ifeq ($(USE_VCD), 1)
	PERLOG += "======== USE_VCD ========\n"
	C_DEFS +=  -DUSE_VCD
endif

# AS includes
AS_INCLUDES =

# C includes
C_INCLUDES = \
        -I /opt/verilator-5.030/share/verilator/include \
        -I $(INCLUDE_DIR)/eyeriss \
        -I $(INCLUDE_DIR)/hal \
        -I $(HW_DIR)/ \
        -I $(TOP_DIR)

######################################
# source
######################################
# C sources
SRC_CPP := $(wildcard $(TOP_DIR)/*.cpp) \
		$(wildcard $(SRC_DIR)/hal/*.cpp) \
        ${wildcard $(SRC_DIR)/eyeriss/dla/*.cpp} \
        ${wildcard $(SRC_DIR)/eyeriss/cpu/original/*.cpp}

SRC_C := $(wildcard $(TOP_DIR)/*.c) \
		$(wildcard $(SRC_DIR)/hal/*.c) \
        ${wildcard $(SRC_DIR)/eyeriss/dla/*.c} \
        ${wildcard $(SRC_DIR)/eyeriss/cpu/original/*.c}

OBJ_CPP := $(addprefix $(OBJ_DIR)/,$(notdir $(SRC_CPP:.cpp=.o)))
OBJ_C := $(addprefix $(OBJ_DIR)/,$(notdir $(SRC_C:.c=.o)))

vpath %.cpp $(sort $(dir $(SRC_CPP)))
vpath %.c $(sort $(dir $(SRC_C)))

SRC := $(SRC_C) $(SRC_CPP)
OBJ := $(OBJ_C) $(OBJ_CPP)

.SUFFIXES: .o .S .cpp

.PHONY: all clean pre test usage

# Default target
usage:
	@echo "Makefile Usage:"
	@echo ""
	@echo "Build the project:"
	@echo "  make all         # Compiles the project and generates the executable"
	@echo ""
	@echo "Enable Debugging and Logging:"
	@echo "  make all DLA_INFO=1   # Enable DLA information logging"
	@echo "  make all DEBUG=1      # Enable debugging mode"
	@echo "  make all USE_VCD=1    # Enable waveform dump (VCD format)"
	@echo ""
	@echo "You can combine options, e.g.:"
	@echo "  make all DLA_INFO=1 DEBUG=1 USE_VCD=1"
	@echo ""
	@echo "Run the program:"
	@echo "  make test        # Executes the built program and logs output"
	@echo ""
	@echo "Clean the build files:"
	@echo "  make clean       # Removes all generated object files and binaries"
	@echo ""
	@echo "Run nWave for debugging:"
	@echo "  make nWave       # Opens nWave for waveform analysis"
	@echo ""
	@echo "To see this help message again, run:"
	@echo "  make [usage]"
	@echo ""

# Make 'usage' the default target
.DEFAULT_GOAL := usage

all: pre $(ELF_NAME)

pre:
	$(QUIET)echo $(PERLOG)

$(ELF_NAME): $(OBJ) | $(OBJ_DIR)
	$(QUIET)echo LD $(ELF_NAME)
	$(QUIET)cd $(OBJ_DIR) && \
	$(CXX) $(notdir $^) $(LDFLAGS) -o ../$(ELF_NAME)

# Rule for compiling C++ files
$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	$(QUIET)echo CXX $(notdir $@)
	$(QUIET)$(CXX) -c $(CFLAGS) $(C_DEFS) $(C_INCLUDES) $^ -o $@

# Rule for compiling C files
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(QUIET)echo CC $(notdir $@)
	$(QUIET)$(CC) -c $(CFLAGS) $(C_DEFS) $(C_INCLUDES) $^ -o $@

$(OBJ_DIR):
	$(QUIET)mkdir -p $@

clean:
	$(QUIET)rm -rvf $(OBJ_DIR)
	$(QUIET)rm -rvf $(ELF_NAME) *.o *.vcd *.fsdb *.log nWave_log *.csv

nWave:
	$(QUIET)echo "Run nWave"
	$(QUIET)mkdir -p nWave_log
	$(QUIET)cd nWave_log && nWave &

test: all
	$(QUIET)echo "Run test"
	$(QUIET)bash -c "./$(ELF_NAME) 1> >(tee out.log) 2> >(tee err.log >&2)"
