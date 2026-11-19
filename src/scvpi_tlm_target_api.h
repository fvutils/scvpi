#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Simulation-thread-only ABI. pop returns 1 for a request, 0 for an empty
// endpoint, -1 for invalid arguments/an unknown endpoint. copy/response return
// 0 on success; negative values denote invalid endpoint/arguments (-1), stale
// ID (-2), invalid delivery state/copy buffer (-3), or response status (-4).
int scvpi_tlm_target_pop_request(const char* endpoint,
                                 uint64_t* id,
                                 int* command,
                                 uint64_t* address,
                                 unsigned int* streaming_width,
                                 unsigned int* data_length,
                                 unsigned int* byte_enable_length);
int scvpi_tlm_target_copy_request(const char* endpoint,
                                   uint64_t id,
                                   unsigned char* data,
                                   unsigned int data_capacity,
                                   unsigned char* byte_enable,
                                   unsigned int byte_enable_capacity);
int scvpi_tlm_target_respond(const char* endpoint,
                              uint64_t id,
                              const unsigned char* data,
                              unsigned int data_length,
                              int response_status);

#ifdef __cplusplus
}
#endif
