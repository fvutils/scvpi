/*
 * ScVpiCallbackBase.h
 *
 *  Created on: Nov 14, 2020
 *      Author: mballance
 */

#pragma once
#include "vpi_user_int.h"
#include "ScVpiHandle.h"

namespace scvpi {

class ScVpiCallbackBase : public ScVpiHandle {

public:
	ScVpiCallbackBase(p_cb_data cbd);

	virtual ~ScVpiCallbackBase();

	void invoke();

protected:
	s_cb_data			m_cb_data;
};

}
