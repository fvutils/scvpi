SYSTEMC ?= /opt/systemc
PYTHON ?= python3
PYTHON_BIN := $(shell command -v $(PYTHON))
BUILD_DIR ?= build
SCVPI_DIR := ../..

COCOTB_CMD := $(PYTHON) -m cocotb_tools.config
COCOTB_LIBPYTHON := $(shell $(COCOTB_CMD) --libpython 2>/dev/null)
COCOTB_LIBPYTHON_DIR := $(dir $(COCOTB_LIBPYTHON))
COCOTB_PYGPI_ENTRY := $(shell $(COCOTB_CMD) --pygpi-entry-point 2>/dev/null)
COCOTB_VPI := $(shell $(COCOTB_CMD) --lib-name-path vpi icarus 2>/dev/null)
SYSTEMC_LIB_DIR := $(firstword $(foreach d,lib-linux64 lib64 lib,$(if $(wildcard $(SYSTEMC)/$(d)),$(SYSTEMC)/$(d))))

.PHONY: build run test-native clean

build:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_PREFIX_PATH=$(SYSTEMC) -DPython3_EXECUTABLE="$(PYTHON_BIN)"
	cmake --build $(BUILD_DIR) --parallel

test-native: build
	ctest --test-dir $(BUILD_DIR) -R tlm_target_native --output-on-failure

run: build
	@test -n "$(COCOTB_LIBPYTHON)" || (echo "cocotb is unavailable; install SCVPI Python dependencies first"; exit 1)
	@test -x "$(BUILD_DIR)/tlm_target_cocotb" || (echo "Python development files are required for the cocotb example"; exit 1)
	rm -f "$(abspath $(BUILD_DIR))/results.xml"
	cd $(BUILD_DIR) && env \
		PYTHONPATH="$(abspath ../../python):$(abspath tests):$$PYTHONPATH" \
		PYTHONHOME= \
		PYGPI_PYTHON_BIN="$(PYTHON_BIN)" \
		GPI_USERS="$(COCOTB_LIBPYTHON);$(COCOTB_PYGPI_ENTRY)" \
		COCOTB_TEST_MODULES=test_target \
		COCOTB_RESULTS_FILE="$(abspath $(BUILD_DIR))/results.xml" \
		TOPLEVEL=top TOPLEVEL_LANG=verilog \
		LD_LIBRARY_PATH="$(COCOTB_LIBPYTHON_DIR):$(SYSTEMC_LIB_DIR):$$LD_LIBRARY_PATH" \
		GPI_EXTRA="$(abspath $(BUILD_DIR))/libscvpi_tlm_target_ext.so:scvpi_tlm_target_ext_entry" \
		./tlm_target_cocotb +vpi="$(COCOTB_VPI)"
	$(PYTHON) ../../tools/check_cocotb_results.py "$(abspath $(BUILD_DIR))/results.xml"

clean:
	rm -rf $(BUILD_DIR) results.xml
