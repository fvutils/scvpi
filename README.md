# SCVPI

SCVPI is a VPI-compatible interface for SystemC that enables **cocotb**
and cocotb-based verification frameworks such as **PyUVM** to run
directly on SystemC models.

By providing a VPI-compatible layer, SCVPI allows Python verification
environments to be reused across HDL RTL, SystemC RTL, and SystemC TLM
models, enabling a unified verification flow across multiple languages
and abstraction levels.

------------------------------------------------------------------------

## Motivation

Python-based verification frameworks such as cocotb and PyUVM are widely
used for RTL verification because of their productivity and reusability.
However, these frameworks rely on simulator interfaces such as the
Verilog Procedural Interface (VPI) and therefore cannot interact
directly with SystemC models.

SystemC is commonly used for architectural exploration, virtual
prototyping, and transaction-level modeling. As a result, verification
environments are often duplicated between HDL RTL and SystemC models.
Although these models represent the same architectural functionality,
they are typically verified using separate environments, resulting in
duplicated effort and inconsistent verification behavior across the
design flow.

SCVPI bridges this gap by exposing a VPI-compatible interface for
SystemC, allowing existing cocotb and PyUVM verification environments to
run on SystemC models with little or no modification.

------------------------------------------------------------------------

## Features

- VPI-compatible interface for SystemC
- Coroutine scheduling required by cocotb
- Signal access and callback support
- Transaction-level communication support for TLM-2.0 blocking
  transport (`b_transport`)
- Reuse of Python verification environments across HDL RTL, SystemC RTL,
  and SystemC TLM models

> **Note**
>
> The current TLM implementation supports blocking transport
> (`b_transport`) in both annotated-delay and target-side wait timing
> modes. Support for non-blocking transport (`nb_transport_fw` /
> `nb_transport_bw`) is planned for a future release.

------------------------------------------------------------------------

## Supported SystemC Types

SCVPI currently supports direct value access for the `sc_signal<T>`
types used by the provided examples, including:

- `sc_signal<bool>`
- `sc_signal<int>`
- selected `sc_signal<sc_int<N>>`
- selected `sc_signal<sc_uint<N>>`

Additional signal types can be added as needed.

------------------------------------------------------------------------

## Requirements

- SystemC 3.0.2
- CMake 3.15 or newer
- Python 3.12 or newer
- uv

Install the Python dependencies (including **cocotb 2.x** and
**PyUVM**) using:

```bash
uv sync
source .venv/bin/activate
```

For the provided HDL example (`examples/tinyalu_reg`), install Icarus
Verilog so that both `iverilog` and `vvp` are available on your `PATH`.

------------------------------------------------------------------------

## Examples

### examples/basic

Minimal example demonstrating cocotb coroutine scheduling on a SystemC
model.

### examples/adder

Simple functional example using deterministic and randomized cocotb
tests.

### examples/TinyALU_SystemC

A SystemC implementation of the official PyUVM TinyALU example.

### examples/tinyalu_reg

Demonstrates reuse of the same PyUVM register-model verification
environment across HDL RTL, SystemC RTL, and SystemC TLM
implementations.

------------------------------------------------------------------------

## Quick Start

```bash
export SYSTEMC=/path/to/systemc

git clone https://github.com/fvutils/scvpi.git
cd scvpi

uv sync
source .venv/bin/activate
```

Run the examples:

```bash
make -C examples/basic run RANDOM_SEED=12345
make -C examples/adder run RANDOM_SEED=12345
make -C examples/TinyALU_SystemC run RANDOM_SEED=12345

make -C examples/tinyalu_reg run-hdl RANDOM_SEED=12345
make -C examples/tinyalu_reg run-sc-rtl RANDOM_SEED=12345
make -C examples/tinyalu_reg run-sc-tlm RANDOM_SEED=12345
make -C examples/tinyalu_reg run-sc-tlm-target-wait RANDOM_SEED=12345
```

> **Note**
>
> The `basic`, `adder`, and `TinyALU_SystemC` examples execute compiled
> SystemC models through SCVPI. Internally, SCVPI provides the
> VPI-compatible interface required by cocotb, allowing the Python
> testbench to interact directly with the SystemC executable.
>
> The `run-hdl` target compiles and simulates the SystemVerilog TinyALU
> using Icarus Verilog, and therefore requires the `iverilog` and `vvp`
> executables to be installed and available on your `PATH`.
>
> The `run-sc-rtl`, `run-sc-tlm`, and `run-sc-tlm-target-wait` targets
> execute compiled SystemC models through SCVPI. The `run-sc-tlm` target
> uses annotated-delay timing, while `run-sc-tlm-target-wait` enables
> target-side wait timing.

------------------------------------------------------------------------

## License

This project is released under the applicable open-source license.
Third-party examples and testbenches retain their original licenses.
