import cocotb
import scvpi_tlm_target_ext
from cocotb.triggers import RisingEdge, Timer, with_timeout

from scvpi_tlm_target import (
    TLM_OK_RESPONSE,
    TLM_READ_COMMAND,
    TLM_WRITE_COMMAND,
    TlmTarget,
    TlmResponse,
)


@cocotb.test()
async def target_round_trip(dut):
    target = TlmTarget("top.target", dut.target.request_toggle)
    registers = {}
    handled = []
    handler_errors = []

    try:
        scvpi_tlm_target_ext.try_recv("top.missing")
    except RuntimeError:
        pass
    else:
        assert False, "an unknown target must not look like an empty queue"

    async def handle(request):
        handled.append(request)
        await Timer(3, unit="ns")
        if request.address == 0xBAD:
            raise ValueError("deliberate peripheral failure")
        assert request.streaming_width == 4
        assert request.byte_en is None
        if request.command == TLM_WRITE_COMMAND:
            registers[request.address] = request.data
            return TlmResponse()
        if request.command == TLM_READ_COMMAND:
            return TlmResponse(data=registers[request.address])
        return TlmResponse(response_status=-3)

    async def serve():
        try:
            await target.serve(handle)
        except ValueError as exc:
            handler_errors.append(str(exc))

    server = cocotb.start_soon(serve())
    await with_timeout(RisingEdge(dut.done), 100, "ns")

    assert len(handled) == 3
    assert handled[0].command == TLM_WRITE_COMMAND
    assert handled[0].address == 0x100000010
    assert handled[1].command == TLM_READ_COMMAND
    assert handled[0].data == bytes.fromhex("11223344")
    assert handled[1].data == bytes(4)
    assert handler_errors == ["deliberate peripheral failure"]
    await server
