/*
 * scvpi.cpp
 *
 *  Created on: Nov 13, 2020
 *      Author: mballance
 */

#include "scvpi.h"
#include <stdio.h>
#include <vector>
#include <string.h>
#ifndef _WIN32
// TODO: need dynamic linking on macOS as well
#include <dlfcn.h>
#else
#error "Not supported on Windows currently"
#endif

#include "scvpi_timed_callback.hpp"
#include "ScVpiCallbackBase.h"

namespace scvpi {

ScVpi::ScVpi(const sc_core::sc_module_name &name) : sc_module(name) {
	// TODO Auto-generated constructor stub

}

ScVpi::~ScVpi() {
	// TODO Auto-generated destructor stub
}

void ScVpi::end_of_elaboration() {

	for (uint32_t i=1; i<sc_core::sc_argc(); i++) {
		if (!strncmp(sc_core::sc_argv()[i], "+vpi=", 4)) {
			const char *filename = &sc_core::sc_argv()[i][5];

			fprintf(stdout, "Load VPI: %s\n", filename);
			void *hndl = dlopen(filename, RTLD_LAZY);

			if (!hndl) {
				fprintf(stdout, "Failed to load library %s\n",
						dlerror());
				return;
			}

			void *vlog_startup_routines_p = dlsym(
					hndl, "vlog_startup_routines");

			if (!vlog_startup_routines_p) {
				fprintf(stdout, "Failed to find vlog_startup_routines: %s\n",
						dlerror());
				return;
			}

            typedef void (*init_f)();
            init_f *startup_f = (init_f *)vlog_startup_routines_p;
            for (int i=0; startup_f[i]; i++) {
            	fprintf(stdout, "--> Calling startup\n");
            	startup_f[i]();
            	fprintf(stdout, "<-- Calling startup\n");
            }
		}
	}

	fprintf(stdout, "ScVpi::end_of_elaboration\n");
}

void ScVpi::start_of_simulation() {
	fprintf(stdout, "--> ScVpi::start_of_simulation\n");
	for (std::vector<ScVpiCallbackBaseUP>::const_iterator
			it=m_start_of_sim_cb.begin();
			it!=m_start_of_sim_cb.end(); it++) {
		(*it)->invoke();
	}

	fprintf(stdout, "<-- ScVpi::start_of_simulation\n");
}

void ScVpi::end_of_simulation() {
	fprintf(stdout, "--> ScVpi::end_of_simulation\n");
	for (std::vector<ScVpiCallbackBaseUP>::const_iterator
			it=m_end_of_sim_cb.begin();
			it!=m_end_of_sim_cb.end(); it++) {
		(*it)->invoke();
	}

	fprintf(stdout, "<-- ScVpi::end_of_simulation\n");
}

void ScVpi::add_start_of_sim_cb(ScVpiCallbackBase *cb) {
	m_start_of_sim_cb.push_back(ScVpiCallbackBaseUP(cb));
}

void ScVpi::add_end_of_sim_cb(ScVpiCallbackBase *cb) {
	m_end_of_sim_cb.push_back(ScVpiCallbackBaseUP(cb));
}

ScVpi *ScVpi::inst() {
	return &m_inst;;
}

//
ScVpi ScVpi::m_inst("__scvpi_plugin");

}

#include "vpi.icc"
#include "ScVpiCallbackBase.icc"
#include "ScVpiHandle.icc"
#include "ScVpiHandleIterator.icc"
#include "ScVpiHandleObject.icc"
#include "ScVpiTimedCallback.icc"

