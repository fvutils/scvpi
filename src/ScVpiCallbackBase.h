/*
 * ScVpiCallbackBase.h
 *
 *  Created on: Nov 14, 2020
 *      Author: mballance
 */

#pragma once
#include "vpi_user_int.h"

namespace scvpi {

class ScVpiCallbackBase {

public:
	ScVpiCallbackBase(p_cb_data cbd);

	virtual ~ScVpiCallbackBase();

	void invoke();

protected:
	s_cb_data			m_cb_data;
};

}
