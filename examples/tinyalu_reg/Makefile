# ==============================================================================
# tinyalu_reg example
# ==============================================================================
# Supported DUT implementations:
#   MODEL=hdl  - HDL RTL through cocotb's simulator Makefile flow
#   MODEL=rtl  - SystemC RTL model through SCVPI
#   MODEL=tlm  - SystemC TLM model through the SCVPI TLM extension
#
# Public run targets:
#   run-hdl                 - HDL RTL model
#   run-sc-rtl              - SystemC RTL model
#   run-sc-tlm              - SystemC TLM model with annotated delay
#   run-sc-tlm-target-wait  - SystemC TLM model with target-side waits
# ==============================================================================

SYSTEMC ?= /usr
SYSTEMC_HOME ?= $(SYSTEMC)

PYTHON ?= $(shell command -v python3)
SIM_KIND ?= icarus
MODEL ?= rtl
SCVPI_LOG ?= 0
TRACE ?= 0
TRACE_DIR ?= traces
PLUSARGS ?=
RANDOM_SEED ?= 12345
TLM_TARGET_CONSUMES_TIME ?= 0

empty :=
space := $(empty) $(empty)

BUILD_DIR := build_$(MODEL)
MODEL_DIR := model_$(MODEL)
SIM_EXE := $(BUILD_DIR)/TINYALU_REG_SC

COCOTB_CMD := $(PYTHON) -m cocotb_tools.config
COCOTB_LIB_DIR := $(shell $(COCOTB_CMD) --lib-dir 2>/dev/null)
VPI_SO := $(shell $(COCOTB_CMD) --lib-name-path vpi icarus 2>/dev/null)

COCOTB_LIBPYTHON := $(shell $(COCOTB_CMD) --libpython 2>/dev/null)
COCOTB_LIBPYTHON_DIR := $(dir $(COCOTB_LIBPYTHON))
COCOTB_PYGPI_ENTRY := $(shell $(COCOTB_CMD) --pygpi-entry-point 2>/dev/null)

RESULTS_FILE := $(CURDIR)/results_$(MODEL).xml
RESULTS_CHECKER := $(CURDIR)/../../tools/check_cocotb_results.py

ifeq ($(MODEL),tlm)
BUS_BACKEND := tlm
GPI_EXTRA_VALUE := $(CURDIR)/$(BUILD_DIR)/libscvpi_tlm_ext.so:scvpi_tlm_ext_entry
else
BUS_BACKEND := rtl
GPI_EXTRA_VALUE :=
endif

ifeq ($(MODEL),hdl)
SIM_MODE := hdl
TOPLEVEL := tinyalu
TOPLEVEL_LANG := verilog
else ifeq ($(MODEL),rtl)
SIM_MODE := systemc
TOPLEVEL := top
TOPLEVEL_LANG := verilog
else ifeq ($(MODEL),tlm)
SIM_MODE := systemc
TOPLEVEL := top
TOPLEVEL_LANG := verilog
else
$(error Unsupported MODEL=$(MODEL). Use MODEL=hdl, MODEL=rtl, or MODEL=tlm)
endif

SYSTEMC_LIB_DIRS := $(foreach d,lib-linux64 lib64 lib,$(if $(wildcard $(SYSTEMC_HOME)/$(d)),$(SYSTEMC_HOME)/$(d)))
LD_PATH_VALUE := $(COCOTB_LIBPYTHON_DIR):$(subst $(space),:,$(SYSTEMC_LIB_DIRS)):$(LD_LIBRARY_PATH)

CMAKE_ARGS := \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_PREFIX_PATH=$(SYSTEMC_HOME) \
	-DSystemC_LIBRARY=$(firstword $(SYSTEMC_LIB_DIRS))/libsystemc.so \
	-DSystemC_INCLUDE_DIR=$(SYSTEMC_HOME)/include

ifeq ($(MODEL),tlm)
CMAKE_ARGS += \
	-DPython3_EXECUTABLE=$(PYTHON) \
	-DTLM_TARGET_CONSUMES_TIME=$(TLM_TARGET_CONSUMES_TIME)
endif

.PHONY: all
all: run

.PHONY: run
run: run-sc-rtl

.PHONY: build
build:
ifeq ($(SIM_MODE),hdl)
	@mkdir -p $(BUILD_DIR)
else
	cmake -S $(MODEL_DIR) -B $(BUILD_DIR) $(CMAKE_ARGS)
	cmake --build $(BUILD_DIR) -j$$(nproc)
endif

# Internal model runner. Use one of the public run-* targets below.
.PHONY: _run-model
_run-model: build
ifeq ($(SIM_MODE),hdl)
	@$(MAKE) -C model_hdl \
		PYTHON="$(PYTHON)" \
		SIM=$(SIM_KIND) \
		TOPLEVEL=$(TOPLEVEL) \
		TOPLEVEL_LANG=$(TOPLEVEL_LANG) \
		COCOTB_TEST_MODULES=tests.testbench \
		COCOTB_TESTCASE=$(TESTCASE) \
		COCOTB_RANDOM_SEED=$(RANDOM_SEED) \
		PLUSARGS="$(PLUSARGS) $(if $(filter 1,$(TRACE)),+trace +trace_file=$(TRACE_DIR)/hdl.vcd,)"
else
	@echo "Running tinyalu_reg ($(MODEL))..."
	@rm -f $(RESULTS_FILE)
	@env \
		PATH="$$(dirname "$(PYTHON)"):$$PATH" \
		PYTHONHOME= \
		PYGPI_PYTHON_BIN="$(PYTHON)" \
		GPI_USERS="$(COCOTB_LIBPYTHON);$(COCOTB_PYGPI_ENTRY)" \
		COCOTB_TEST_MODULES=tests.testbench \
		COCOTB_RESULTS_FILE="$(RESULTS_FILE)" \
		COCOTB_RANDOM_SEED="$(RANDOM_SEED)" \
		$(if $(TESTCASE),COCOTB_TESTCASE="$(TESTCASE)",) \
		TOPLEVEL="$(TOPLEVEL)" \
		TOPLEVEL_LANG="$(TOPLEVEL_LANG)" \
		BUS_BACKEND="$(BUS_BACKEND)" \
		TLM_TARGET_CONSUMES_TIME="$(TLM_TARGET_CONSUMES_TIME)" \
		SCVPI_LOG="$(SCVPI_LOG)" \
		SC_TRACE="$(TRACE)" \
		SC_TRACE_FILE="$(TRACE_DIR)/$(MODEL)" \
		TLM_TRACE="$(TRACE)" \
		PYTHONPATH="$(CURDIR)/tests:$(CURDIR):$$PYTHONPATH" \
		LD_LIBRARY_PATH="$(LD_PATH_VALUE)" \
		$(if $(GPI_EXTRA_VALUE),GPI_EXTRA="$(GPI_EXTRA_VALUE)",) \
		"./$(SIM_EXE)" +vpi="$(VPI_SO)" $(PLUSARGS)
	@$(PYTHON) $(RESULTS_CHECKER) $(RESULTS_FILE)
endif

.PHONY: run-hdl
run-hdl:
	$(MAKE) _run-model \
		MODEL=hdl \
		SIM_KIND=$(SIM_KIND) \
		RANDOM_SEED=$(RANDOM_SEED)

.PHONY: run-sc-rtl
run-sc-rtl:
	$(MAKE) _run-model \
		MODEL=rtl \
		RANDOM_SEED=$(RANDOM_SEED)

.PHONY: run-sc-tlm
run-sc-tlm:
	$(MAKE) _run-model \
		MODEL=tlm \
		RANDOM_SEED=$(RANDOM_SEED) \
		TLM_TARGET_CONSUMES_TIME=0

.PHONY: run-sc-tlm-target-wait
run-sc-tlm-target-wait:
	$(MAKE) _run-model \
		MODEL=tlm \
		RANDOM_SEED=$(RANDOM_SEED) \
		TLM_TARGET_CONSUMES_TIME=1

.PHONY: trace
trace:
	$(MAKE) _run-model \
		MODEL=$(MODEL) \
		SIM_KIND=$(SIM_KIND) \
		TRACE=1 \
		RANDOM_SEED=$(RANDOM_SEED) \
		TLM_TARGET_CONSUMES_TIME=$(TLM_TARGET_CONSUMES_TIME)

.PHONY: show-config
show-config:
	@echo "MODEL=$(MODEL)"
	@echo "SYSTEMC_HOME=$(SYSTEMC_HOME)"
	@echo "SYSTEMC_LIB_DIRS=$(SYSTEMC_LIB_DIRS)"
	@echo "PYTHON=$(PYTHON)"
	@echo "SIM_KIND=$(SIM_KIND)"
	@echo "MODEL_DIR=$(MODEL_DIR)"
	@echo "BUILD_DIR=$(BUILD_DIR)"
	@echo "BUS_BACKEND=$(BUS_BACKEND)"
	@echo "TRACE=$(TRACE)"
	@echo "RANDOM_SEED=$(RANDOM_SEED)"
	@echo "TLM_TARGET_CONSUMES_TIME=$(TLM_TARGET_CONSUMES_TIME)"
	@echo "VPI_SO=$(VPI_SO)"
	@echo "GPI_EXTRA=$(GPI_EXTRA_VALUE)"

.PHONY: help
help:
	@echo "Available targets:"
	@echo "  run                     - Run the default SystemC RTL model"
	@echo "  run-hdl                 - Run the HDL RTL model"
	@echo "  run-sc-rtl              - Run the SystemC RTL model"
	@echo "  run-sc-tlm              - Run the SystemC TLM model with annotated delay"
	@echo "  run-sc-tlm-target-wait  - Run the SystemC TLM model with target-side waits"
	@echo "  trace                   - Run MODEL=$(MODEL) with tracing enabled"
	@echo "  show-config             - Display the active configuration"
	@echo "  clean                   - Remove generated files"
	@echo ""
	@echo "Common variables:"
	@echo "  RANDOM_SEED=<n>         - Set the cocotb random seed"
	@echo "  TESTCASE=<name>         - Run one cocotb test"
	@echo "  SIM_KIND=<simulator>    - Select the HDL simulator for run-hdl"
	@echo "  MODEL=hdl|rtl|tlm       - Select the model used by trace"
	@echo "  TRACE_DIR=<path>        - Set the trace output directory"

.PHONY: clean
clean:
	rm -rf build_hdl build_rtl build_tlm traces results*.xml sim_build __pycache__ tests/__pycache__
	$(MAKE) -C model_hdl clean PYTHON="$(PYTHON)" SIM=icarus || true