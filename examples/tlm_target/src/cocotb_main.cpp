#include "scvpi_tlm_target.h"

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>

#include <array>
#include <cstdint>
#include <iostream>

SC_MODULE(Top) {
    scvpi::TlmTarget target;
    tlm_utils::simple_initiator_socket<Top, 32> initiator;
    sc_core::sc_signal<bool> done;

    SC_CTOR(Top) : target("target"), initiator("initiator"), done("done") {
        initiator.bind(target.socket);
        SC_THREAD(run_transactions);
    }

    void run_transactions() {
        std::array<unsigned char, 4> data{0x11, 0x22, 0x33, 0x44};
        tlm::tlm_generic_payload payload;
        payload.set_address(UINT64_C(0x100000010));
        payload.set_data_ptr(data.data());
        payload.set_data_length(static_cast<unsigned int>(data.size()));
        payload.set_streaming_width(static_cast<unsigned int>(data.size()));
        payload.set_command(tlm::TLM_WRITE_COMMAND);
        payload.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        sc_core::sc_time delay(5, sc_core::SC_NS);
        const auto start = sc_core::sc_time_stamp();
        initiator->b_transport(payload, delay);
        sc_assert(payload.get_response_status() == tlm::TLM_OK_RESPONSE);
        sc_assert(delay == sc_core::SC_ZERO_TIME);
        sc_assert(sc_core::sc_time_stamp() - start == sc_core::sc_time(8, sc_core::SC_NS));

        data.fill(0);
        payload.set_command(tlm::TLM_READ_COMMAND);
        payload.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        initiator->b_transport(payload, delay);
        sc_assert(payload.get_response_status() == tlm::TLM_OK_RESPONSE);
        sc_assert(data[0] == 0x11 && data[1] == 0x22 && data[2] == 0x33 && data[3] == 0x44);

        // A failing Python handler must still release the blocked initiator.
        payload.set_address(0xBAD);
        payload.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        initiator->b_transport(payload, delay);
        sc_assert(payload.get_response_status() == tlm::TLM_GENERIC_ERROR_RESPONSE);

        std::cout << "PASS: cocotb-serviced SystemC TLM target\n";
        done.write(true);
        // Let cocotb observe done and finish its assertions before a fallback stop.
        wait(10, sc_core::SC_NS);
        sc_core::sc_stop();
    }
};

extern "C" void scvpi_autoregister_all();

int sc_main(int, char **) {
    Top top("top");
    scvpi_autoregister_all();
    sc_core::sc_start();
    return 0;
}
