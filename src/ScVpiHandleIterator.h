/*
 * ScVpiObjectIterator.h
 *
 *  Created on: Nov 14, 2020
 *      Author: mballance
 */

#pragma once
#include <stdint.h>
#include <vector>
#include "ScVpiHandle.h"
#include "ScVpiHandleObject.h"

namespace scvpi {

class ScVpiHandleIterator : public ScVpiHandle {
public:
	ScVpiHandleIterator(
			const char *kind,
			const std::vector<sc_core::sc_object*> &items);

	virtual ~ScVpiHandleIterator();

	ScVpiHandleObject *next();

private:
	const char									*m_kind;
	const std::vector<sc_core::sc_object*>		&m_items;
	uint32_t									m_index;
	ScVpiHandleObject							m_item;
};

}
