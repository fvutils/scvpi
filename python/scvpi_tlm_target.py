"""Cocotb helpers for SystemC-initiated SCVPI TLM target requests."""

from __future__ import annotations

from dataclasses import dataclass
from collections.abc import Awaitable, Callable

import scvpi_tlm_target_ext
from cocotb.triggers import ValueChange

TLM_READ_COMMAND = 0
TLM_WRITE_COMMAND = 1
TLM_IGNORE_COMMAND = 2

TLM_OK_RESPONSE = 1
TLM_INCOMPLETE_RESPONSE = 0
TLM_GENERIC_ERROR_RESPONSE = -1
TLM_ADDRESS_ERROR_RESPONSE = -2
TLM_COMMAND_ERROR_RESPONSE = -3
TLM_BURST_ERROR_RESPONSE = -4
TLM_BYTE_ENABLE_ERROR_RESPONSE = -5


@dataclass(frozen=True)
class TlmRequest:
    id: int
    command: int
    address: int
    streaming_width: int
    data: bytes
    byte_en: bytes | None

    @property
    def is_read(self) -> bool:
        return self.command == TLM_READ_COMMAND


@dataclass(frozen=True)
class TlmResponse:
    data: bytes = b""
    response_status: int = TLM_OK_RESPONSE


class TlmTarget:
    """Receive blocking TLM requests from a SystemC ``scvpi::TlmTarget``.

    ``request_toggle`` must be that target's VPI-visible SystemC signal handle.
    The SystemC target waits until :meth:`respond` completes each request.
    """

    def __init__(self, endpoint: str, request_toggle) -> None:
        self.endpoint = endpoint
        self._request_toggle = request_toggle

    def _try_recv(self) -> TlmRequest | None:
        item = scvpi_tlm_target_ext.try_recv(self.endpoint)
        if item is None:
            return None
        request_id, command, address, streaming_width, data, byte_en = item
        return TlmRequest(
            id=int(request_id),
            command=int(command),
            address=int(address),
            streaming_width=int(streaming_width),
            data=bytes(data),
            byte_en=bytes(byte_en) if byte_en else None,
        )

    async def recv(self) -> TlmRequest:
        """Wait for and return the next request, checking the queue before sleeping."""
        while True:
            request = self._try_recv()
            if request is not None:
                return request
            await ValueChange(self._request_toggle)

    def respond(
        self,
        request: TlmRequest,
        data: bytes = b"",
        response_status: int = TLM_OK_RESPONSE,
    ) -> None:
        """Complete a request. A read response must contain the requested data bytes."""
        scvpi_tlm_target_ext.respond(
            self.endpoint,
            request.id,
            bytes(data),
            int(response_status),
        )

    async def serve(
        self, handler: Callable[[TlmRequest], Awaitable[TlmResponse]]
    ) -> None:
        """Serve requests serially; fail an in-flight request if its handler raises.

        Handler coroutines may consume service latency using cocotb triggers.
        Exceptions (including cancellation) are propagated after attempting an
        error response. There must be only one receiver per endpoint.
        """
        while True:
            request = await self.recv()
            try:
                response = await handler(request)
                self.respond(request, response.data, response.response_status)
            except BaseException:
                try:
                    self.respond(request, response_status=TLM_GENERIC_ERROR_RESPONSE)
                except RuntimeError:
                    # Preserve the original failure if the native timeout fired.
                    pass
                raise
