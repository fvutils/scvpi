/*
 * scvpi_timed_callback.hpp
 *
 *  Created on: Nov 14, 2020
 *      Author: mballance
 */

#pragma once
#include <systemc>

namespace scvpi {

class timed_callback : public sc_core::sc_process_host {
public:

	timed_callback(const sc_core::sc_time &t) : m_count(0), m_time(t) {
        ::sc_core::sc_process_handle handle =
            sc_core::sc_get_curr_simcontext()->create_method_process(
                0,  true, SC_MAKE_FUNC_PTR(timed_callback, callback),
                this, 0);
	}

private:
	void callback() {
		fprintf(stdout, "callback: count=%d\n", m_count);
		if (m_count == 0) {
			// We're called immediately, and reschedule ourselves
			next_trigger(m_time, sc_core::sc_get_curr_simcontext());
		}
		m_count++;
	}


private:
	uint32_t				m_count;
	sc_core::sc_time		m_time;
};

}

