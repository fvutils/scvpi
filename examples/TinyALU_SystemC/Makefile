SYSTEMC ?= /usr
CXX ?= g++
RANDOM_SEED ?= 12345

ROOT ?= ../..
SCVPI_SRC := $(ROOT)/src

empty :=
space := $(empty) $(empty)
SYSTEMC_LIB_DIRS := $(foreach d,lib-linux64 lib64 lib,$(if $(wildcard $(SYSTEMC)/$(d)),$(SYSTEMC)/$(d)))
SYSTEMC_LDFLAGS := $(foreach d,$(SYSTEMC_LIB_DIRS),-L$(d))
SYSTEMC_LD_PATH := $(subst $(space),:,$(SYSTEMC_LIB_DIRS))

CXXFLAGS += -I$(SYSTEMC)/include -I./hdl -I$(SCVPI_SRC) -export-dynamic
LDFLAGS  += $(SYSTEMC_LDFLAGS) -ldl -lsystemc

COCOTB_CMD           = $(shell command -v uv >/dev/null 2>&1 && echo "uv run cocotb-config" || echo "cocotb-config")
COCOTB_LIBPYTHON     = $(shell $(COCOTB_CMD) --libpython 2>/dev/null || echo "")
COCOTB_LIBPYTHON_DIR = $(dir $(COCOTB_LIBPYTHON))
COCOTB_PYTHON_BIN    = $(shell $(COCOTB_CMD) --python-bin 2>/dev/null || echo "python3")
COCOTB_LIB_DIR       = $(shell $(COCOTB_CMD) --lib-dir 2>/dev/null || echo "")
COCOTB_PYGPI_ENTRY   = $(shell $(COCOTB_CMD) --pygpi-entry-point 2>/dev/null || echo "")
COCOTB_VPI_ICARUS    = $(shell $(COCOTB_CMD) --lib-name-path vpi icarus 2>/dev/null || echo "")

RESULTS_FILE    := $(CURDIR)/results.xml
RESULTS_CHECKER := $(CURDIR)/../../tools/check_cocotb_results.py

COCOTB_ENV = \
	PYTHONPATH=.:$$PYTHONPATH \
	PYGPI_PYTHON_BIN=$(COCOTB_PYTHON_BIN) \
	GPI_USERS="$(COCOTB_LIBPYTHON);$(COCOTB_PYGPI_ENTRY)" \
	COCOTB_TEST_MODULES=testbench \
	COCOTB_RESULTS_FILE=$(RESULTS_FILE) \
	COCOTB_RANDOM_SEED=$(RANDOM_SEED) \
	LD_LIBRARY_PATH=$(SYSTEMC_LD_PATH):$(COCOTB_LIBPYTHON_DIR):$$LD_LIBRARY_PATH

SCVPI_SRCS := \
	$(SCVPI_SRC)/scvpi.cpp \
	$(SCVPI_SRC)/scvpi_tlm_bridge.cpp

.PHONY: all
all: run

.PHONY: test
test: hdl/main.cpp hdl/tinyalu_systemc.h $(SCVPI_SRCS)
	$(CXX) $(CXXFLAGS) $(SCVPI_SRCS) hdl/main.cpp -o test $(LDFLAGS)

.PHONY: run
run: test
	@echo "Running TinyALU SystemC test..."
	@rm -f $(RESULTS_FILE)
	@$(COCOTB_ENV) ./test -m $(COCOTB_VPI_ICARUS)
	@$(COCOTB_PYTHON_BIN) $(RESULTS_CHECKER) $(RESULTS_FILE)

.PHONY: clean
clean:
	rm -f test results.xml

.PHONY: show-config
show-config:
	@echo "=== TinyALU SystemC Configuration ==="
	@echo "SYSTEMC: $(SYSTEMC)"
	@echo "SYSTEMC_LIB_DIRS: $(SYSTEMC_LIB_DIRS)"
	@echo "CXX: $(CXX)"
	@echo "SCVPI_SRC: $(SCVPI_SRC)"
	@echo ""
	@echo "=== cocotb Configuration ==="
	@echo "COCOTB_PYTHON_BIN: $(COCOTB_PYTHON_BIN)"
	@echo "RANDOM_SEED: $(RANDOM_SEED)"
	@echo "COCOTB_LIBPYTHON: $(COCOTB_LIBPYTHON)"
	@echo "COCOTB_VPI_ICARUS: $(COCOTB_VPI_ICARUS)"