/*
 * ScVpiHandlObject.h
 *
 *  Created on: Nov 15, 2020
 *      Author: mballance
 */

#pragma once
#include "ScVpiHandle.h"

namespace sc_core {
    class sc_object;
}

namespace scvpi {

class ScVpiHandleObject : public ScVpiHandle {
public:
	ScVpiHandleObject();

	ScVpiHandleObject(sc_core::sc_object *obj);

	virtual ~ScVpiHandleObject();

	const char *name() const;

	sc_core::sc_object *obj() const { return m_obj; }

	void obj(sc_core::sc_object *obj) { m_obj = obj; }

	int32_t getVpiType() const;

private:
	sc_core::sc_object					*m_obj;

};

}
