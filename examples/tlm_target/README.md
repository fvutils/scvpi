# SystemC-initiated TLM target

This example demonstrates **SystemC initiator → SCVPI target → cocotb
coroutine → TLM response**. Peripheral behavior can live in Python; no
direct call into Python is made from the initiator's `b_transport()`.

## Run

Use the same C++ standard and SystemC library as the host executable
(C++17 and SystemC 3.0.2 for this example). CMake discovers SystemC through
its installed `SystemCLanguage` package. Python development headers and a
shared Python library are required for the cocotb example.

From the SCVPI repository root, with cocotb 2.1 installed in the selected
Python environment:

```sh
make -C examples/tlm_target test-native SYSTEMC=/path/to/systemc
make -C examples/tlm_target run SYSTEMC=/path/to/systemc PYTHON=/path/to/python
```

`BUILD_DIR` may be a relative or absolute path. `test-native` does not
require Python development files or cocotb. `run` checks cocotb's results
XML and fails if the test fails or no results are produced. No HDL
simulator executable is required: the SystemC executable loads cocotb's
VPI library directly.

## C++ target

Include `scvpi_tlm_target.h`, instantiate `scvpi::TlmTarget`, and bind a
32-bit TLM initiator socket to its `socket`. The target name, for example
`top.target`, identifies its Python endpoint. The constructor accepts an
optional positive response timeout; the default is 1 ms simulated time.

Link `src/scvpi_tlm_target.cpp` **once, into the executable**, alongside
the existing `scvpi.cpp` and `scvpi_tlm_bridge.cpp`. Export executable
symbols (`ENABLE_EXPORTS` in CMake). Build `scvpi_tlm_target_ext.cpp` as a
shared library linked to Python, but do not link another copy of the
target implementation into it: that would create a second endpoint
registry. The Makefile shows its `GPI_EXTRA` entry point and runtime
Python paths.

## Python model

Add SCVPI's `python/` directory to `PYTHONPATH`, then:

```python
from cocotb.triggers import Timer
from scvpi_tlm_target import TlmTarget, TlmResponse, TLM_WRITE_COMMAND

target = TlmTarget("top.target", dut.target.request_toggle)
registers = {}

async def handle(request):
    await Timer(3, unit="ns")
    if request.command == TLM_WRITE_COMMAND:
        registers[request.address] = request.data
        return TlmResponse()
    return TlmResponse(data=registers.get(request.address, bytes(len(request.data))))

await target.serve(handle)
```

`TlmRequest` contains `id`, `command`, a 64-bit `address`, `streaming_width`,
`data` bytes, and optional `byte_en` bytes. `TlmResponse` contains `data`
and `response_status` (defaults to `TLM_OK_RESPONSE`). The helper exports
the standard TLM command and response-status constants. The model is
responsible for register semantics, streaming, and applying byte enables.

Alternatively, use `await target.recv()` followed by
`target.respond(request, data, response_status)` to manage the service
loop yourself. Use exactly **one receiver per target**. `serve()` attempts
a generic error response if the handler fails or is cancelled, then
re-raises the original exception. Start it as a cocotb task and await it
or let task failures propagate to the test.

## Scheduling and timing contract

- The caller must be a SystemC **`SC_THREAD`**, not an `SC_METHOD`, host
  thread, elaboration callback, or a suspended synchronous Python call
  through the existing forward-TLM extension.
- Incoming TLM delay is consumed with `wait(delay)` before enqueueing.
  The returned delay is zero. Handler waits consume additional simulation
  time; do not annotate that latency a second time.
- Multiple callers may queue requests. A single SystemC worker publishes
  and services them FIFO, **one in-flight request per target**. Separate
  targets can progress independently.
- The worker toggles `request_toggle` to wake cocotb through SCVPI's
  normal VPI callbacks. `recv()` checks the queue before sleeping, so a
  responder that starts after publication does not lose the request.
- Requests own copies of data and byte enables. A successful read must
  return exactly the requested number of bytes, or the initiator receives
  `TLM_BURST_ERROR_RESPONSE`. Error responses leave the caller's data
  unchanged. DMI is always disabled.
- The watchdog starts when a request is published, not while it is queued
  or consuming its incoming delay. It returns `TLM_GENERIC_ERROR_RESPONSE`
  if unanswered. Late, duplicate, invalid-status, and unknown-ID responses
  are rejected; request IDs are not reused during the target's lifetime.
- All API calls and target lifetime operations belong to the simulation
  thread. The API does not provide OS-thread synchronization, reset or
  transaction cancellation. Simulation stop terminates outstanding
  service; no response is promised after stop.
- Nonblocking TLM, debug transport, generic-payload extensions, DMI, and
  out-of-order responses are not implemented. SCVPI currently exposes VPI
  time in nanoseconds; use integer-nanosecond cocotb timers. Its read-only
  and read/write synchronization callbacks are delta waits, not full HDL
  scheduling regions.

The native test exercises the C ABI, payload copying, byte enables,
streaming width, 64-bit addresses, error/status validation, concurrent
callers, timeouts, late responses, and consumed timing. The cocotb test
exercises a Python register model, service latency, and handler failure.