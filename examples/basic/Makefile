# ==============================================================================
# SystemC + cocotb Test Makefile
# ==============================================================================

SYSTEMC ?= /usr
CXX ?= g++
RANDOM_SEED ?= 12345
SIM ?= icarus

# Repo root relative to this Makefile (examples/basic/)
ROOT ?= ../..
SCVPI_SRC := $(ROOT)/src

# ------------------------------------------------------------------------------
# Compiler and Linker Configuration
# ------------------------------------------------------------------------------
BASE_CXXFLAGS = -I$(SYSTEMC)/include -I. -I$(SCVPI_SRC) -export-dynamic
BASE_LDFLAGS  = -ldl -lsystemc

# SystemC library paths
empty :=
space := $(empty) $(empty)
SYSTEMC_LIB_DIRS := $(foreach d,lib-linux64 lib64 lib,$(if $(wildcard $(SYSTEMC)/$(d)),$(SYSTEMC)/$(d)))
SYSTEMC_LDFLAGS := $(foreach d,$(SYSTEMC_LIB_DIRS),-L$(d))
SYSTEMC_LD_PATH := $(subst $(space),:,$(SYSTEMC_LIB_DIRS))

# ------------------------------------------------------------------------------
# cocotb Configuration
# ------------------------------------------------------------------------------
COCOTB_CMD           = $(shell command -v uv >/dev/null 2>&1 && echo "uv run cocotb-config" || echo "cocotb-config")
COCOTB_PREFIX        = $(shell $(COCOTB_CMD) --prefix 2>/dev/null || echo "")
COCOTB_LIBPYTHON     = $(shell $(COCOTB_CMD) --libpython 2>/dev/null || echo "")
COCOTB_LIBPYTHON_DIR = $(dir $(COCOTB_LIBPYTHON))
COCOTB_PYTHON_BIN    = $(shell $(COCOTB_CMD) --python-bin 2>/dev/null || echo "python3")
COCOTB_LIB_DIR       = $(shell $(COCOTB_CMD) --lib-dir 2>/dev/null || echo "")
COCOTB_PYGPI_ENTRY   = $(shell $(COCOTB_CMD) --pygpi-entry-point 2>/dev/null || echo "")

# VPI library paths
COCOTB_VPI_VERILATOR = $(shell $(COCOTB_CMD) --lib-name-path vpi verilator 2>/dev/null || echo "")
COCOTB_VPI_QUESTA    = $(shell $(COCOTB_CMD) --lib-name-path vpi questa 2>/dev/null || echo "")
COCOTB_VPI_XCELIUM   = $(shell $(COCOTB_CMD) --lib-name-path vpi xcelium 2>/dev/null || echo "")
COCOTB_VPI_ICARUS    = $(shell $(COCOTB_CMD) --lib-name-path vpi icarus 2>/dev/null || echo "")

# ------------------------------------------------------------------------------
# Build Configurations
# ------------------------------------------------------------------------------
CXXFLAGS = $(BASE_CXXFLAGS)
LDFLAGS  = $(SYSTEMC_LDFLAGS) $(BASE_LDFLAGS)

# Simulator-specific linker flags
VERILATOR_LDFLAGS = $(SYSTEMC_LDFLAGS) $(BASE_LDFLAGS)
QUESTA_LDFLAGS    = $(SYSTEMC_LDFLAGS) $(BASE_LDFLAGS)
XCELIUM_LDFLAGS   = $(SYSTEMC_LDFLAGS) $(BASE_LDFLAGS)
ICARUS_LDFLAGS    = $(SYSTEMC_LDFLAGS) $(BASE_LDFLAGS)

# ------------------------------------------------------------------------------
# cocotb Runtime Environment
# ------------------------------------------------------------------------------
BASE_COCOTB_ENV = \
	PYTHONPATH=.:$$PYTHONPATH \
	PYGPI_PYTHON_BIN=$(COCOTB_PYTHON_BIN) \
	GPI_USERS="$(COCOTB_LIBPYTHON);$(COCOTB_PYGPI_ENTRY)" \
	COCOTB_TEST_MODULES=tb \
	COCOTB_RESULTS_FILE=$(RESULTS_FILE) \
	COCOTB_RANDOM_SEED=$(RANDOM_SEED)

define MAKE_COCOTB_ENV
$(BASE_COCOTB_ENV) \
	LD_LIBRARY_PATH=$(SYSTEMC_LD_PATH):$(COCOTB_LIBPYTHON_DIR):$$LD_LIBRARY_PATH
endef

VERILATOR_ENV = $(call MAKE_COCOTB_ENV)
QUESTA_ENV    = $(call MAKE_COCOTB_ENV)
XCELIUM_ENV   = $(call MAKE_COCOTB_ENV)
ICARUS_ENV    = $(call MAKE_COCOTB_ENV)

# ------------------------------------------------------------------------------
# Helper Paths
# ------------------------------------------------------------------------------
RESULTS_FILE    := $(CURDIR)/results.xml
RESULTS_CHECKER := $(CURDIR)/../../tools/check_cocotb_results.py

# ------------------------------------------------------------------------------
# Build Targets
# ------------------------------------------------------------------------------
SCVPI_SRCS := \
	$(SCVPI_SRC)/scvpi.cpp \
	$(SCVPI_SRC)/scvpi_tlm_bridge.cpp

.PHONY: all
all: run

.PHONY: run
run: run-$(SIM)

.PHONY: test
test: test.cpp $(SCVPI_SRCS)
	$(CXX) $(CXXFLAGS) $(SCVPI_SRCS) test.cpp -o test $(LDFLAGS)

# Simulator-specific builds
.PHONY: test-verilator test-questa test-xcelium test-icarus
test-verilator test-questa test-xcelium test-icarus: test

# ------------------------------------------------------------------------------
# Simulator Targets
# ------------------------------------------------------------------------------
.PHONY: verilator questa xcelium icarus
verilator: LDFLAGS = $(VERILATOR_LDFLAGS)
verilator: test-verilator

questa: LDFLAGS = $(QUESTA_LDFLAGS)
questa: test-questa

xcelium: LDFLAGS = $(XCELIUM_LDFLAGS)
xcelium: test-xcelium

icarus: LDFLAGS = $(ICARUS_LDFLAGS)
icarus: test-icarus

# ------------------------------------------------------------------------------
# Debug Build Targets
# ------------------------------------------------------------------------------
.PHONY: debug
debug: CXXFLAGS += -DEN_DEBUG_VPI
debug: test

.PHONY: verilator-debug questa-debug xcelium-debug icarus-debug
verilator-debug: CXXFLAGS = $(BASE_CXXFLAGS) -DEN_DEBUG_VPI
verilator-debug: LDFLAGS  = $(VERILATOR_LDFLAGS)
verilator-debug: test

questa-debug: CXXFLAGS = $(BASE_CXXFLAGS) -DEN_DEBUG_VPI
questa-debug: LDFLAGS  = $(QUESTA_LDFLAGS)
questa-debug: test

xcelium-debug: CXXFLAGS = $(BASE_CXXFLAGS) -DEN_DEBUG_VPI
xcelium-debug: LDFLAGS  = $(XCELIUM_LDFLAGS)
xcelium-debug: test

icarus-debug: CXXFLAGS = $(BASE_CXXFLAGS) -DEN_DEBUG_VPI
icarus-debug: LDFLAGS  = $(ICARUS_LDFLAGS)
icarus-debug: test

# ------------------------------------------------------------------------------
# Run Targets
# ------------------------------------------------------------------------------
define make_run_target
.PHONY: run-$(1)
run-$(1): test-$(1)
	@echo "Running cocotb test with $(2)..."
	@rm -f $(RESULTS_FILE)
	@$(3) ./test $(4)
	@$(COCOTB_PYTHON_BIN) $(RESULTS_CHECKER) $(RESULTS_FILE)

.PHONY: run-$(1)-debug
run-$(1)-debug: $(1)-debug
	@echo "Running cocotb test with $(2) (DEBUG)..."
	@rm -f $(RESULTS_FILE)
	@$(3) ./test $(4)
	@$(COCOTB_PYTHON_BIN) $(RESULTS_CHECKER) $(RESULTS_FILE)
endef

$(eval $(call make_run_target,verilator,Verilator,$(VERILATOR_ENV),+vpi=$(COCOTB_VPI_VERILATOR)))
$(eval $(call make_run_target,questa,Questa,$(QUESTA_ENV),+vpi=$(COCOTB_VPI_QUESTA)))
$(eval $(call make_run_target,xcelium,Xcelium,$(XCELIUM_ENV),+vpi=$(COCOTB_VPI_XCELIUM)))
$(eval $(call make_run_target,icarus,Icarus,$(ICARUS_ENV),-m $(COCOTB_VPI_ICARUS)))

.PHONY: run-debug
run-debug: debug
	./test

# ------------------------------------------------------------------------------
# Utility Targets
# ------------------------------------------------------------------------------
.PHONY: clean
clean:
	rm -f test results.xml /tmp/cocotb_*.log

.PHONY: show-config
show-config:
	@echo "=== SystemC Configuration ==="
	@echo "SYSTEMC: $(SYSTEMC)"
	@echo "SYSTEMC_LIB_DIRS: $(SYSTEMC_LIB_DIRS)"
	@echo ""
	@echo "=== cocotb Configuration ==="
	@echo "COCOTB_PREFIX: $(COCOTB_PREFIX)"
	@echo "COCOTB_LIBPYTHON: $(COCOTB_LIBPYTHON)"
	@echo "COCOTB_LIBPYTHON_DIR: $(COCOTB_LIBPYTHON_DIR)"
	@echo "COCOTB_PYTHON_BIN: $(COCOTB_PYTHON_BIN)"
	@echo "RANDOM_SEED: $(RANDOM_SEED)"
	@echo ""
	@echo "=== VPI Libraries ==="
	@echo "COCOTB_VPI_VERILATOR: $(COCOTB_VPI_VERILATOR)"
	@echo "COCOTB_VPI_QUESTA: $(COCOTB_VPI_QUESTA)"
	@echo "COCOTB_VPI_XCELIUM: $(COCOTB_VPI_XCELIUM)"
	@echo "COCOTB_VPI_ICARUS: $(COCOTB_VPI_ICARUS)"

.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all               - Run the selected simulator test"
	@echo "  run               - Run with the selected simulator (default: SIM=$(SIM))"
	@echo "  test              - Build the test executable"
	@echo ""
	@echo "Simulator builds:"
	@echo "  verilator         - Build for Verilator"
	@echo "  questa            - Build for Questa"
	@echo "  xcelium           - Build for Xcelium"
	@echo "  icarus            - Build for Icarus"
	@echo ""
	@echo "Simulator-specific runs:"
	@echo "  run-verilator     - Run with Verilator VPI"
	@echo "  run-questa        - Run with Questa VPI"
	@echo "  run-xcelium       - Run with Xcelium VPI"
	@echo "  run-icarus        - Run with Icarus VPI"
	@echo ""
	@echo "Debug builds:"
	@echo "  debug             - Build with debug flags"
	@echo "  *-debug           - Build simulator-specific debug"
	@echo "  run-*-debug       - Run simulator-specific debug"
	@echo ""
	@echo "Utilities:"
	@echo "  clean             - Remove built files, results, and logs"
	@echo "  show-config       - Display configuration"
	@echo "  help              - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  SIM=<simulator>   - Simulator selected by the run target"
	@echo "                      Supported: icarus, verilator, questa, xcelium"
	@echo "  RANDOM_SEED=<n>   - Set the cocotb random seed"