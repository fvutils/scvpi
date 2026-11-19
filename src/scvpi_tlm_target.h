#pragma once

#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>

#include "scvpi_tlm_target_api.h"

namespace scvpi {

struct TlmTargetRequest {
    uint64_t id = 0;
    int command = 0;
    uint64_t address = 0;
    std::vector<unsigned char> data;
    std::vector<unsigned char> byte_en;
    unsigned int streaming_width = 0;
};

// A blocking TLM target whose requests are serviced by a cocotb coroutine.
// Calls must originate from a SystemC process that may wait (e.g. SC_THREAD).
class TlmTarget : public sc_core::sc_module {
public:
    tlm_utils::simple_target_socket<TlmTarget, 32> socket;
    // VPI-visible wake-up toggle. The request FIFO, not this signal, is authoritative.
    sc_core::sc_signal<bool> request_toggle;

    SC_HAS_PROCESS(TlmTarget);
    explicit TlmTarget(sc_core::sc_module_name name,
                       sc_core::sc_time response_timeout = sc_core::sc_time(1, sc_core::SC_MS));
    ~TlmTarget() override;

    bool try_recv(TlmTargetRequest& request);
    bool copy_request(uint64_t id,
                      std::vector<unsigned char>& data,
                      std::vector<unsigned char>& byte_en) const;
    int respond(uint64_t id,
                const std::vector<unsigned char>& data,
                int response_status);

private:
    struct PendingRequest {
        TlmTargetRequest request;
        sc_core::sc_event completed_event;
        std::vector<unsigned char> response_data;
        tlm::tlm_response_status response_status = tlm::TLM_INCOMPLETE_RESPONSE;
        bool completed = false;
        bool published = false;
        bool delivered = false;
    };

    void b_transport(tlm::tlm_generic_payload& payload, sc_core::sc_time& delay);
    void publish_requests();

    std::deque<std::shared_ptr<PendingRequest>> requests_;
    std::map<uint64_t, std::shared_ptr<PendingRequest>> pending_;
    sc_core::sc_event request_available_;
    uint64_t next_id_ = 1;
    sc_core::sc_time response_timeout_;
};

// Called by the Python extension on the SystemC simulation thread.
bool tlm_target_try_recv(const std::string& endpoint, TlmTargetRequest& request);
int tlm_target_respond(const std::string& endpoint,
                       uint64_t id,
                       const std::vector<unsigned char>& data,
                       int response_status);
} // namespace scvpi
