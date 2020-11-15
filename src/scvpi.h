/*
 * scvpi.h
 *
 *  Created on: Nov 13, 2020
 *      Author: mballance
 */

#pragma once
#include <memory>
#include <vector>
#include <systemc>

namespace scvpi {

class ScVpiCallbackBase;
typedef std::unique_ptr<ScVpiCallbackBase> ScVpiCallbackBaseUP;

class ScVpi : public sc_core::sc_module {
public:
	ScVpi(const sc_core::sc_module_name &name);

	virtual ~ScVpi();

	virtual void end_of_elaboration();

	virtual void start_of_simulation() override;

    virtual void end_of_simulation() override;

	void add_start_of_sim_cb(ScVpiCallbackBase *cb);

	void add_end_of_sim_cb(ScVpiCallbackBase *cb);

	virtual const char *kind() const {
		return "scvpi_plugin";
	}

	static ScVpi *inst();

private:
	std::vector<ScVpiCallbackBaseUP>		m_start_of_sim_cb;
	std::vector<ScVpiCallbackBaseUP>		m_end_of_sim_cb;

	static ScVpi		m_inst;

};


}

