/*
 * ScVpiHandle.h
 *
 *  Created on: Nov 15, 2020
 *      Author: mballance
 */

#pragma once

namespace scvpi {

enum ScVpiHandleKind {
	HandleKind_Callback,
	HandleKind_Iterator,
	HandleKind_Object
};


class ScVpiHandle {
public:
	ScVpiHandle(ScVpiHandleKind kind);

	virtual ~ScVpiHandle();

	ScVpiHandleKind kind() const { return m_kind; }

private:
	ScVpiHandleKind				m_kind;

};

}

