#include "scvpi_tlm_target.h"

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {
constexpr const char *endpoint = "top.target";
constexpr const char *timeout_endpoint = "top.timeout_target";
constexpr uint64_t write_addr = UINT64_C(0xfedcba9876543210);
constexpr uint64_t read_addr = UINT64_C(0x8123456789abcdef);
constexpr uint64_t ignore_addr = 0x30;
constexpr uint64_t mismatch_addr = 0x40;
constexpr uint64_t empty_addr = 0x50;
constexpr uint64_t error_base = 0x100;
constexpr uint64_t concurrent_base = 0x200;
const std::array<unsigned char, 5> write_data{{0x00, 0x80, 0xff, 0x42, 0x00}};
const std::array<unsigned char, 3> byte_en{{0xff, 0x00, 0x80}};
const std::array<unsigned char, 5> read_data{{0xff, 0x00, 0x81, 0x00, 0x7f}};
const std::array<int, 5> errors{{tlm::TLM_GENERIC_ERROR_RESPONSE, tlm::TLM_ADDRESS_ERROR_RESPONSE,
                                 tlm::TLM_COMMAND_ERROR_RESPONSE, tlm::TLM_BURST_ERROR_RESPONSE,
                                 tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE}};

unsigned int checks = 0;
void check(bool condition, const char *expression, int line) {
    ++checks;
    if (!condition) {
        const std::string message = std::string("line ") + std::to_string(line) + ": " + expression;
        SC_REPORT_FATAL("native_test", message.c_str());
    }
}
#define CHECK(expression) check((expression), #expression, __LINE__)

struct Request {
    uint64_t id = 0;
    int command = 0;
    uint64_t address = 0;
    unsigned int width = 0;
    unsigned int length = 0;
    unsigned int byte_length = 0;
};

int pop(const char *name, Request &r) {
    return scvpi_tlm_target_pop_request(name, &r.id, &r.command, &r.address, &r.width, &r.length,
                                        &r.byte_length);
}
} // namespace

SC_MODULE(Top) {
    scvpi::TlmTarget target;
    scvpi::TlmTarget timeout_target;
    tlm_utils::simple_initiator_socket<Top, 32> initiator;
    tlm_utils::simple_initiator_socket<Top, 32> timeout_initiator;
    sc_core::sc_event start_concurrent, concurrent_done;
    sc_core::sc_time expected_read_arrival = sc_core::SC_ZERO_TIME;
    unsigned int started = 0;
    unsigned int completed = 0;
    unsigned int scenarios = 0;
    uint64_t timeout_id = 0;
    std::vector<uint64_t> concurrent_order;
    bool success = false;

    SC_CTOR(Top)
        : target("target", sc_core::sc_time(100, sc_core::SC_NS)),
          timeout_target("timeout_target", sc_core::sc_time(20, sc_core::SC_NS)),
          initiator("initiator"), timeout_initiator("timeout_initiator") {
        // Multiple caller threads can share this bound initiator socket.
        initiator.bind(target.socket);
        timeout_initiator.bind(timeout_target.socket);
        SC_THREAD(responder);
        SC_THREAD(timeout_observer);
        SC_THREAD(run);
        SC_THREAD(caller1);
        SC_THREAD(caller2);
    }

    void responder() {
        // The first request is already published before the responder starts.
        // Checking the FIFO before waiting avoids a lost toggle notification.
        wait(sc_core::sc_time(5, sc_core::SC_NS));
        bool first = true;
        while (true) {
            Request r;
            if (pop(endpoint, r) == 0) {
                CHECK(!first);
                wait(target.request_toggle.value_changed_event());
                continue;
            }
            if (first) {
                CHECK(sc_core::sc_time_stamp() == sc_core::sc_time(5, sc_core::SC_NS));
                CHECK(r.address == write_addr);
                first = false;
            }
            CHECK(r.id != 0);
            Request unavailable;
            CHECK(pop(endpoint, unavailable) == 0);
            std::vector<unsigned char> data(r.length), enables(r.byte_length);
            CHECK(scvpi_tlm_target_copy_request(endpoint, r.id, data.data(), r.length,
                                                enables.data(), r.byte_length) == 0);

            if (r.address == write_addr) {
                CHECK(r.command == tlm::TLM_WRITE_COMMAND);
                CHECK(r.width == 2 && r.length == write_data.size());
                CHECK(r.byte_length == byte_en.size());
                CHECK(std::vector<unsigned char>(write_data.begin(), write_data.end()) == data);
                CHECK(std::vector<unsigned char>(byte_en.begin(), byte_en.end()) == enables);
                CHECK(scvpi_tlm_target_copy_request(endpoint, r.id, data.data(), r.length - 1,
                                                    enables.data(), r.byte_length) == -3);
                CHECK(scvpi_tlm_target_copy_request(endpoint, r.id, data.data(), r.length,
                                                    enables.data(), r.byte_length - 1) == -3);
                CHECK(scvpi_tlm_target_copy_request(endpoint, r.id, nullptr, r.length,
                                                    enables.data(), r.byte_length) == -3);
                CHECK(scvpi_tlm_target_copy_request(endpoint, r.id, data.data(), r.length, nullptr,
                                                    r.byte_length) == -3);
                CHECK(scvpi_tlm_target_respond(endpoint, r.id, nullptr, 1, tlm::TLM_OK_RESPONSE) ==
                      -1);
                CHECK(scvpi_tlm_target_respond(endpoint, r.id, nullptr, 0,
                                               tlm::TLM_INCOMPLETE_RESPONSE) == -4);
                CHECK(scvpi_tlm_target_respond(endpoint, r.id, nullptr, 0, 12345) == -4);
                CHECK(scvpi_tlm_target_respond(endpoint, r.id, nullptr, 0, -99) == -4);
                CHECK(scvpi_tlm_target_respond(endpoint, r.id + 100000, nullptr, 0,
                                               tlm::TLM_OK_RESPONSE) == -2);
                // Even a nonempty response must not overwrite a WRITE buffer.
                CHECK(scvpi_tlm_target_respond(endpoint, r.id, read_data.data(), read_data.size(),
                                               tlm::TLM_OK_RESPONSE) == 0);
                CHECK(scvpi_tlm_target_respond(endpoint, r.id, nullptr, 0, tlm::TLM_OK_RESPONSE) ==
                      -3);
                CHECK(scvpi_tlm_target_copy_request(endpoint, r.id, data.data(), r.length,
                                                    enables.data(), r.byte_length) == -2);
            } else if (r.address == read_addr) {
                CHECK(r.command == tlm::TLM_READ_COMMAND && r.width == 3);
                CHECK(sc_core::sc_time_stamp() == expected_read_arrival);
                CHECK(data == std::vector<unsigned char>(write_data.begin(), write_data.end()));
                CHECK(enables == std::vector<unsigned char>(byte_en.begin(), byte_en.end()));
                wait(sc_core::sc_time(3, sc_core::SC_NS));
                CHECK(scvpi_tlm_target_respond(endpoint, r.id, read_data.data(), read_data.size(),
                                               tlm::TLM_OK_RESPONSE) == 0);
            } else if (r.address == ignore_addr) {
                CHECK(r.command == tlm::TLM_IGNORE_COMMAND && r.width == 1);
                CHECK(data == std::vector<unsigned char>(write_data.begin(), write_data.end()));
                CHECK(scvpi_tlm_target_respond(endpoint, r.id, read_data.data(), read_data.size(),
                                               tlm::TLM_OK_RESPONSE) == 0);
            } else if (r.address == mismatch_addr) {
                CHECK(r.command == tlm::TLM_READ_COMMAND);
                CHECK(scvpi_tlm_target_respond(endpoint, r.id, read_data.data(),
                                               read_data.size() - 1, tlm::TLM_OK_RESPONSE) == 0);
            } else if (r.address == empty_addr) {
                CHECK(r.command == tlm::TLM_READ_COMMAND && r.length == 0 && r.byte_length == 0);
                CHECK(scvpi_tlm_target_copy_request(endpoint, r.id, nullptr, 0, nullptr, 0) == 0);
                CHECK(scvpi_tlm_target_respond(endpoint, r.id, nullptr, 0, tlm::TLM_OK_RESPONSE) ==
                      0);
            } else if (r.address >= error_base && r.address < error_base + errors.size()) {
                CHECK(r.command == tlm::TLM_READ_COMMAND);
                CHECK(scvpi_tlm_target_respond(endpoint, r.id, read_data.data(), read_data.size(),
                                               errors[r.address - error_base]) == 0);
            } else {
                CHECK(r.address == concurrent_base || r.address == concurrent_base + 1);
                CHECK(r.command == tlm::TLM_WRITE_COMMAND);
                concurrent_order.push_back(r.address);
                // Both callers have queued, but the second must not be published yet.
                wait(sc_core::sc_time(4, sc_core::SC_NS));
                CHECK(started == 2 && completed == concurrent_order.size() - 1);
                CHECK(pop(endpoint, unavailable) == 0);
                CHECK(scvpi_tlm_target_respond(endpoint, r.id, nullptr, 0, tlm::TLM_OK_RESPONSE) ==
                      0);
            }
        }
    }

    void timeout_observer() {
        wait(timeout_target.request_toggle.value_changed_event());
        Request r;
        CHECK(pop(timeout_endpoint, r) == 1);
        CHECK(r.address == 0xdead && r.command == tlm::TLM_WRITE_COMMAND);
        timeout_id = r.id;
        // Deliberately never respond here.
    }

    void concurrent_call(uint64_t address) {
        wait(start_concurrent);
        ++started;
        unsigned char data = static_cast<unsigned char>(address);
        tlm::tlm_generic_payload p;
        p.set_command(tlm::TLM_WRITE_COMMAND);
        p.set_address(address);
        p.set_data_ptr(&data);
        p.set_data_length(1);
        p.set_streaming_width(1);
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        initiator->b_transport(p, delay);
        CHECK(p.get_response_status() == tlm::TLM_OK_RESPONSE);
        CHECK(delay == sc_core::SC_ZERO_TIME);
        ++completed;
        concurrent_done.notify(sc_core::SC_ZERO_TIME);
    }
    void caller1() {
        concurrent_call(concurrent_base);
    }
    void caller2() {
        concurrent_call(concurrent_base + 1);
    }

    void run() {
        Request r;
        CHECK(pop("top.missing", r) == -1);
        CHECK(pop(nullptr, r) == -1);
        CHECK(scvpi_tlm_target_pop_request(endpoint, nullptr, &r.command, &r.address, &r.width,
                                           &r.length, &r.byte_length) == -1);
        CHECK(scvpi_tlm_target_copy_request(nullptr, 1, nullptr, 0, nullptr, 0) == -1);
        CHECK(scvpi_tlm_target_copy_request("top.missing", 1, nullptr, 0, nullptr, 0) == -2);
        CHECK(scvpi_tlm_target_copy_request(endpoint, 99999, nullptr, 0, nullptr, 0) == -2);
        CHECK(scvpi_tlm_target_respond(nullptr, 1, nullptr, 0, tlm::TLM_OK_RESPONSE) == -1);
        CHECK(scvpi_tlm_target_respond("top.missing", 1, nullptr, 0, tlm::TLM_OK_RESPONSE) == -1);
        ++scenarios;

        auto data = write_data;
        auto enables = byte_en;
        tlm::tlm_generic_payload p;
        p.set_address(write_addr);
        p.set_data_ptr(data.data());
        p.set_data_length(data.size());
        p.set_byte_enable_ptr(enables.data());
        p.set_byte_enable_length(enables.size());
        p.set_streaming_width(2);
        p.set_command(tlm::TLM_WRITE_COMMAND);
        p.set_dmi_allowed(true);
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        initiator->b_transport(p, delay);
        CHECK(p.get_response_status() == tlm::TLM_OK_RESPONSE);
        CHECK(!p.is_dmi_allowed() && delay == sc_core::SC_ZERO_TIME);
        CHECK(data == write_data && enables == byte_en);
        CHECK(sc_core::sc_time_stamp() == sc_core::sc_time(5, sc_core::SC_NS));
        ++scenarios;

        p.set_address(read_addr);
        p.set_command(tlm::TLM_READ_COMMAND);
        p.set_streaming_width(3);
        p.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        delay = sc_core::sc_time(7, sc_core::SC_NS);
        const auto read_start = sc_core::sc_time_stamp();
        expected_read_arrival = read_start + delay;
        initiator->b_transport(p, delay);
        CHECK(p.get_response_status() == tlm::TLM_OK_RESPONSE && data == read_data);
        CHECK(enables == byte_en && delay == sc_core::SC_ZERO_TIME);
        CHECK(sc_core::sc_time_stamp() - read_start == sc_core::sc_time(10, sc_core::SC_NS));
        ++scenarios;

        data = write_data;
        p.set_address(ignore_addr);
        p.set_command(tlm::TLM_IGNORE_COMMAND);
        p.set_streaming_width(1);
        p.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        initiator->b_transport(p, delay);
        CHECK(p.get_response_status() == tlm::TLM_OK_RESPONSE && data == write_data);
        ++scenarios;

        p.set_command(tlm::TLM_READ_COMMAND);
        for (unsigned int i = 0; i < errors.size(); ++i) {
            data = write_data;
            p.set_address(error_base + i);
            p.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
            initiator->b_transport(p, delay);
            CHECK(p.get_response_status() == errors[i] && data == write_data);
            ++scenarios;
        }
        p.set_address(mismatch_addr);
        p.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        initiator->b_transport(p, delay);
        CHECK(p.get_response_status() == tlm::TLM_BURST_ERROR_RESPONSE && data == write_data);
        ++scenarios;

        p.set_data_ptr(nullptr);
        p.set_dmi_allowed(true);
        p.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        initiator->b_transport(p, delay);
        CHECK(p.get_response_status() == tlm::TLM_BURST_ERROR_RESPONSE && !p.is_dmi_allowed());
        ++scenarios;
        p.set_data_ptr(data.data());
        p.set_byte_enable_ptr(nullptr);
        p.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        initiator->b_transport(p, delay);
        CHECK(p.get_response_status() == tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE);
        ++scenarios;
        p.set_byte_enable_ptr(enables.data());
        p.set_byte_enable_length(0);
        p.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        initiator->b_transport(p, delay);
        CHECK(p.get_response_status() == tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE);
        ++scenarios;
        p.set_address(empty_addr);
        p.set_data_ptr(nullptr);
        p.set_data_length(0);
        p.set_byte_enable_ptr(nullptr);
        p.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        initiator->b_transport(p, delay);
        CHECK(p.get_response_status() == tlm::TLM_OK_RESPONSE);
        ++scenarios;

        const auto concurrent_start = sc_core::sc_time_stamp();
        start_concurrent.notify(sc_core::SC_ZERO_TIME);
        while (completed != 2)
            wait(concurrent_done);
        CHECK(concurrent_order.size() == 2 && concurrent_order[0] != concurrent_order[1]);
        CHECK(sc_core::sc_time_stamp() - concurrent_start == sc_core::sc_time(8, sc_core::SC_NS));
        ++scenarios;

        unsigned char timeout_data = 0x80;
        tlm::tlm_generic_payload timeout_payload;
        timeout_payload.set_address(0xdead);
        timeout_payload.set_command(tlm::TLM_WRITE_COMMAND);
        timeout_payload.set_data_ptr(&timeout_data);
        timeout_payload.set_data_length(1);
        timeout_payload.set_streaming_width(1);
        timeout_payload.set_dmi_allowed(true);
        const auto timeout_start = sc_core::sc_time_stamp();
        timeout_initiator->b_transport(timeout_payload, delay);
        CHECK(sc_core::sc_time_stamp() - timeout_start == sc_core::sc_time(20, sc_core::SC_NS));
        CHECK(timeout_payload.get_response_status() == tlm::TLM_GENERIC_ERROR_RESPONSE);
        CHECK(!timeout_payload.is_dmi_allowed() && timeout_data == 0x80 && timeout_id != 0);
        CHECK(scvpi_tlm_target_respond(timeout_endpoint, timeout_id, nullptr, 0,
                                       tlm::TLM_OK_RESPONSE) == -2);
        CHECK(scvpi_tlm_target_copy_request(timeout_endpoint, timeout_id, nullptr, 0, nullptr, 0) ==
              -2);
        CHECK(pop(timeout_endpoint, r) == 0);
        ++scenarios;

        success = true;
        std::cout << "PASS: " << scenarios << " scenarios, " << checks << " checks\n";
        sc_core::sc_stop();
    }
};

int sc_main(int, char **) {
    Top top("top");
    sc_core::sc_start(sc_core::sc_time(1, sc_core::SC_US));
    if (!top.success) {
        std::cerr
            << "FAIL: simulation stopped or watchdog expired before all scenarios completed\n";
        return 1;
    }
    return 0;
}
