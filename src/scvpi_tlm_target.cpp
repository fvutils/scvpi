#include "scvpi_tlm_target.h"

#include <cstring>
#include <stdexcept>

namespace scvpi {
namespace {
using TargetMap = std::map<std::string, TlmTarget *>;

TargetMap &targets() {
    static TargetMap instance;
    return instance;
}
} // namespace

TlmTarget::TlmTarget(sc_core::sc_module_name name, sc_core::sc_time response_timeout)
    : sc_module(name), socket("socket"), request_toggle("request_toggle"),
      response_timeout_(response_timeout) {
    if (response_timeout_ <= sc_core::SC_ZERO_TIME) {
        throw std::invalid_argument("TLM target response timeout must be positive");
    }
    socket.register_b_transport(this, &TlmTarget::b_transport);

    const std::string endpoint(this->name());
    if (!targets().emplace(endpoint, this).second) {
        SC_REPORT_FATAL("SCVPI/TLM", ("Duplicate TLM target: " + endpoint).c_str());
    }

    SC_THREAD(publish_requests);
}

TlmTarget::~TlmTarget() {
    targets().erase(this->name());
}

bool TlmTarget::try_recv(TlmTargetRequest &request) {
    if (requests_.empty()) {
        return false;
    }
    const auto pending = requests_.front();
    if (!pending->published || pending->delivered || pending->completed) {
        return false;
    }
    pending->delivered = true;
    request = pending->request;
    return true;
}

bool TlmTarget::copy_request(uint64_t id, std::vector<unsigned char> &data,
                             std::vector<unsigned char> &byte_en) const {
    const auto pending = pending_.find(id);
    if (pending == pending_.end() || !pending->second->delivered || pending->second->completed) {
        return false;
    }
    data = pending->second->request.data;
    byte_en = pending->second->request.byte_en;
    return true;
}

int TlmTarget::respond(uint64_t id, const std::vector<unsigned char> &data, int response_status) {
    const bool valid_status = response_status == tlm::TLM_OK_RESPONSE ||
                              response_status == tlm::TLM_GENERIC_ERROR_RESPONSE ||
                              response_status == tlm::TLM_ADDRESS_ERROR_RESPONSE ||
                              response_status == tlm::TLM_COMMAND_ERROR_RESPONSE ||
                              response_status == tlm::TLM_BURST_ERROR_RESPONSE ||
                              response_status == tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE;
    if (!valid_status) {
        return -4;
    }
    const auto pending_it = pending_.find(id);
    if (pending_it == pending_.end()) {
        return -2;
    }
    const auto &pending = pending_it->second;
    if (!pending->delivered || pending->completed) {
        return -3;
    }
    pending->response_data = data;
    pending->response_status = static_cast<tlm::tlm_response_status>(response_status);
    pending->completed = true;
    pending->completed_event.notify(sc_core::SC_ZERO_TIME);
    return 0;
}

void TlmTarget::b_transport(tlm::tlm_generic_payload &payload, sc_core::sc_time &delay) {
    payload.set_dmi_allowed(false);
    const auto process = sc_core::sc_get_current_process_handle();
    if (!process.valid() || (process.proc_kind() != sc_core::SC_THREAD_PROC_ &&
                             process.proc_kind() != sc_core::SC_CTHREAD_PROC_)) {
        SC_REPORT_ERROR("SCVPI/TLM",
                        "b_transport must be called from a wait-capable SystemC thread");
        payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
        return;
    }
    // This adapter consumes incoming annotation before handing the transaction
    // to Python. Python's own SystemC-time waits then contribute service time.
    if (delay != sc_core::SC_ZERO_TIME) {
        sc_core::wait(delay);
        delay = sc_core::SC_ZERO_TIME;
    }

    auto pending = std::make_shared<PendingRequest>();
    pending->request.id = next_id_++;
    pending->request.command = static_cast<int>(payload.get_command());
    pending->request.address = payload.get_address();
    pending->request.streaming_width = payload.get_streaming_width();

    const unsigned int length = payload.get_data_length();
    if (length != 0 && payload.get_data_ptr() == nullptr) {
        payload.set_response_status(tlm::TLM_BURST_ERROR_RESPONSE);
        return;
    }
    if (length != 0) {
        pending->request.data.assign(payload.get_data_ptr(), payload.get_data_ptr() + length);
    }

    const unsigned int byte_enable_length = payload.get_byte_enable_length();
    if (payload.get_byte_enable_ptr() != nullptr && byte_enable_length != 0) {
        pending->request.byte_en.assign(payload.get_byte_enable_ptr(),
                                        payload.get_byte_enable_ptr() + byte_enable_length);
    } else if (payload.get_byte_enable_ptr() != nullptr || byte_enable_length != 0) {
        payload.set_response_status(tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE);
        return;
    }

    requests_.push_back(pending);
    pending_.emplace(pending->request.id, pending);
    request_available_.notify(sc_core::SC_ZERO_TIME);

    // TLM b_transport is blocking: keep the caller's payload alive until Python
    // responds or the watchdog expires. This must run in a wait-capable process.
    while (!pending->completed) {
        sc_core::wait(pending->completed_event);
    }

    if (pending->response_status == tlm::TLM_OK_RESPONSE &&
        payload.get_command() == tlm::TLM_READ_COMMAND) {
        if (pending->response_data.size() != length) {
            payload.set_response_status(tlm::TLM_BURST_ERROR_RESPONSE);
            return;
        }
        if (length != 0) {
            std::memcpy(payload.get_data_ptr(), pending->response_data.data(), length);
        }
    }
    payload.set_response_status(pending->response_status);
}

void TlmTarget::publish_requests() {
    while (true) {
        while (requests_.empty()) {
            sc_core::wait(request_available_);
        }

        const auto pending = requests_.front();
        pending->published = true;
        request_toggle.write(!request_toggle.read());

        // Prevent a broken or failed Python responder from hanging simulation
        // forever. A late response is rejected after the request is removed.
        while (!pending->completed) {
            sc_core::wait(response_timeout_, pending->completed_event);
            if (!pending->completed) {
                pending->response_status = tlm::TLM_GENERIC_ERROR_RESPONSE;
                pending->completed = true;
                pending->completed_event.notify(sc_core::SC_ZERO_TIME);
            }
        }

        pending_.erase(pending->request.id);
        requests_.pop_front();
    }
}

bool tlm_target_try_recv(const std::string &endpoint, TlmTargetRequest &request) {
    const auto target = targets().find(endpoint);
    return target != targets().end() && target->second->try_recv(request);
}

int tlm_target_respond(const std::string &endpoint, uint64_t id,
                       const std::vector<unsigned char> &data, int response_status) {
    const auto target = targets().find(endpoint);
    if (target == targets().end()) {
        return -1;
    }

    return target->second->respond(id, data, response_status);
}

} // namespace scvpi

extern "C" int scvpi_tlm_target_pop_request(const char *endpoint, uint64_t *id, int *command,
                                            uint64_t *address, unsigned int *streaming_width,
                                            unsigned int *data_length,
                                            unsigned int *byte_enable_length) {
    if (endpoint == nullptr || id == nullptr || command == nullptr || address == nullptr ||
        streaming_width == nullptr || data_length == nullptr || byte_enable_length == nullptr) {
        return -1;
    }
    if (scvpi::targets().find(endpoint) == scvpi::targets().end()) {
        return -1;
    }

    scvpi::TlmTargetRequest request;
    if (!scvpi::tlm_target_try_recv(endpoint, request)) {
        return 0;
    }

    *id = request.id;
    *command = request.command;
    *address = request.address;
    *streaming_width = request.streaming_width;
    *data_length = static_cast<unsigned int>(request.data.size());
    *byte_enable_length = static_cast<unsigned int>(request.byte_en.size());
    return 1;
}

extern "C" int scvpi_tlm_target_copy_request(const char *endpoint, uint64_t id, unsigned char *data,
                                             unsigned int data_capacity, unsigned char *byte_enable,
                                             unsigned int byte_enable_capacity) {
    if (endpoint == nullptr) {
        return -1;
    }
    std::vector<unsigned char> request_data;
    std::vector<unsigned char> request_byte_enable;
    const auto target = scvpi::targets().find(endpoint);
    if (target == scvpi::targets().end() ||
        !target->second->copy_request(id, request_data, request_byte_enable)) {
        return -2;
    }
    if (request_data.size() > data_capacity || request_byte_enable.size() > byte_enable_capacity ||
        (!request_data.empty() && data == nullptr) ||
        (!request_byte_enable.empty() && byte_enable == nullptr)) {
        return -3;
    }
    if (!request_data.empty()) {
        std::memcpy(data, request_data.data(), request_data.size());
    }
    if (!request_byte_enable.empty()) {
        std::memcpy(byte_enable, request_byte_enable.data(), request_byte_enable.size());
    }
    return 0;
}

extern "C" int scvpi_tlm_target_respond(const char *endpoint, uint64_t id,
                                        const unsigned char *data, unsigned int data_length,
                                        int response_status) {
    if (endpoint == nullptr || (data_length != 0 && data == nullptr)) {
        return -1;
    }
    const std::vector<unsigned char> response =
        data_length == 0 ? std::vector<unsigned char>{}
                         : std::vector<unsigned char>(data, data + data_length);
    return scvpi::tlm_target_respond(endpoint, id, response, response_status);
}
