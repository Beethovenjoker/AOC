VALID_VER := improve original
VALID_OP := CONV CONV_MAX LINEAR_RELU LINEAR

# check if VER is valid
ifdef VER
ifeq ($(filter $(VER),$(VALID_VER)),)
    $(error Invalid VER="$(VER)". Allowed values: improve, original)
endif
endif

# check if OP is valid
ifdef OP
ifeq ($(filter $(OP),$(VALID_OP)),)
    $(error Invalid OP="$(OP)". Allowed values: CONV, CONV_MAX, LINEAR_RELU, LINEAR)
endif
endif

ELF_NAME:= $(OP)_$(VER).elf

export CC ?= gcc
export CXX ?= g++
export VALGRIND ?= valgrind

TARGET := $(ELF_NAME)

PROJECTBASE = $(PWD)
override PROJECTBASE    := $(abspath $(PROJECTBASE))
TOP_DIR = $(PROJECTBASE)

OBJ_DIR := build
LOG_DIR := log
CACHEGRIND_OUTPUT_DIR:= cachegrind_out
MASSIF_OUTPUT_DIR:= massif_out
MASSIF_VIZUALIZER_DIR:= massif_visual
SRC_DIR := $(TOP_DIR)/../../src/eyeriss/cpu/$(VER)
INCLUDE_DIR := $(TOP_DIR)/../../include

C_DEFS = -D$(OP)

# C includes
C_INCLUDES = \
        -I $(INCLUDE_DIR)/eyeriss \
        -I $(TOP_DIR)

# C/CPP sources
SRC_C := $(wildcard $(TOP_DIR)/*.c) \
        ${wildcard $(SRC_DIR)/*.c}

SRC_CPP := $(wildcard $(TOP_DIR)/*.cpp) \
        ${wildcard $(SRC_DIR)/*.cpp}

#.c/.cpp -> .o
OBJ_C := $(addprefix $(OBJ_DIR)/$(OP)_$(VER)_,$(notdir $(SRC_C:.c=.o)))
OBJ_CPP := $(addprefix $(OBJ_DIR)/$(OP)_$(VER)_,$(notdir $(SRC_CPP:.cpp=.o)))

SRC := $(SRC_C) $(SRC_CPP)
OBJ := $(OBJ_C) $(OBJ_CPP)

vpath %.cpp $(sort $(dir $(SRC_CPP)))
vpath %.c $(sort $(dir $(SRC)))

.SUFFIXES: .o .S .cpp .c

.PHONY: all default usage test clean valgrind valgrind_all \
		i_% o_% v_i_% v_o_% \

default: usage

usage:
	@echo "Makefile Usage:"
	@echo ""
	@echo "Run the improved version of an operation:"
	@echo "  make i_<operation>"
	@echo "  Example: make i_conv    # Runs 'make test VER=improve OP=CONV'"
	@echo ""
	@echo "Run the original version of an operation:"
	@echo "  make o_<operation>"
	@echo "  Example: make o_linear  # Runs 'make test VER=original OP=LINEAR'"
	@echo ""
	@echo "Available operations: conv, conv_max, linear, linear_relu."
	@echo ""
	@echo "To verify all operations, run:"
	@echo "  make all"
	@echo ""
	@echo "To use valgrind to test singal operations, run:"
	@echo "  make v_o_<operation>"   # Runs 'make valgrind VER=original OP=<operation>'"
	@echo "  make v_i_<operation>"   # Runs 'make valgrind VER=improve OP=<operation>'"
	@echo ""
	@echo "To use valgrind to test all operations, run:"
	@echo "  make valgrind_all"
	@echo ""
	@echo "To see this help message again, run:"
	@echo "  make usage"
	@echo ""

test: $(TARGET) | $(LOG_DIR)
	@echo ""
	@echo "Running test with VER=$(VER), OP=$(OP)"
	@./$(TARGET) | tee > $(LOG_DIR)/$(TARGET).log

valgrind: $(TARGET) | $(LOG_DIR) $(CACHEGRIND_OUTPUT_DIR) $(MASSIF_OUTPUT_DIR)
	@echo ""
	@echo "Running valgrind with VER=$(VER), OP=$(OP)"

	@$(VALGRIND) --tool=cachegrind \
	--log-file=$(LOG_DIR)/$(ELF_NAME)_cachegrind.log \
	--cachegrind-out-file=$(CACHEGRIND_OUTPUT_DIR)/cachegrind.out.%p_$(ELF_NAME) \
	./$(TARGET)

	@$(VALGRIND) --tool=massif \
	--log-file=$(LOG_DIR)/$(ELF_NAME)_massif.log \
	--massif-out-file=$(MASSIF_OUTPUT_DIR)/massif.out.%p_$(ELF_NAME) \
	./$(TARGET)

$(CACHEGRIND_OUTPUT_DIR):
	mkdir -p $@

$(MASSIF_OUTPUT_DIR):
	mkdir -p $@

$(LOG_DIR):
	mkdir -p $@

$(TARGET): $(OBJ) | $(OBJ_DIR)
	@echo LD  $@
	@$(CC) $(CFLAGS) -o $(TARGET) $^ -lm

$(OBJ_DIR):
	mkdir -p $@

$(OBJ_DIR)/$(OP)_$(VER)_%.o: %.cpp | $(OBJ_DIR)
	@echo "CXX $*"
	@$(CXX) -c $(CFLAGS) $(C_DEFS) $(C_INCLUDES) $< -o $@

$(OBJ_DIR)/$(OP)_$(VER)_%.o: %.c | $(OBJ_DIR)
	@echo "CC $*"
	@$(CC) -c $(CFLAGS) $(C_DEFS) $(C_INCLUDES) $< -o $@

i_%:
	make test VER=improve OP=$(shell echo $* | tr '[:lower:]' '[:upper:]')
o_%:
	make test VER=original OP=$(shell echo $* | tr '[:lower:]' '[:upper:]')
v_i_%:
	make valgrind VER=improve OP=$(shell echo $* | tr '[:lower:]' '[:upper:]')
v_o_%:
	make valgrind VER=original OP=$(shell echo $* | tr '[:lower:]' '[:upper:]')

all:
	make test VER=original OP=CONV
	make test VER=original OP=CONV_MAX
	make test VER=original OP=LINEAR
	make test VER=original OP=LINEAR_RELU
	make test VER=improve OP=CONV
	make test VER=improve OP=CONV_MAX
	make test VER=improve OP=LINEAR
	make test VER=improve OP=LINEAR_RELU

valgrind_all:
	make valgrind VER=original OP=CONV
	make valgrind VER=original OP=CONV_MAX
	make valgrind VER=original OP=LINEAR
	make valgrind VER=original OP=LINEAR_RELU
	make valgrind VER=improve OP=CONV
	make valgrind VER=improve OP=CONV_MAX
	make valgrind VER=improve OP=LINEAR
	make valgrind VER=improve OP=LINEAR_RELU

massif_visualizer:
	mkdir -p $(MASSIF_VIZUALIZER_DIR) && cd $(MASSIF_VIZUALIZER_DIR) && massif-visualizer &

clean:
	rm -rf $(OBJ_DIR)
	rm -rf $(LOG_DIR)
	rm -rf $(CACHEGRIND_OUTPUT_DIR)
	rm -rf $(MASSIF_OUTPUT_DIR)
	rm -rf $(MASSIF_VIZUALIZER_DIR)
	rm -rf $(ELF_NAME) *.o
	rm -rf CONV_*
	rm -rf LINEAR_*
	rm -rf massif.out*
	rm -rf cachegrind.out*
