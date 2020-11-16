/*
 * ScVpiTimedCallback.h
 *
 *  Created on: Nov 15, 2020
 *      Author: mballance
 */

#pragma once
#include <memory>
#include <systemc>
#include "ScVpiCallbackBase.h"

namespace scvpi {
class sc_timed_callback;
typedef std::unique_ptr<sc_timed_callback> sc_timed_callback_up;

class ScVpiTimedCallback : public ScVpiCallbackBase {
public:
	ScVpiTimedCallback(p_cb_data cbd);

	virtual ~ScVpiTimedCallback();

private:

	sc_timed_callback_up			m_cb;;
};

} /* namespace scvpi */

