
#include "systemc.h"
// #include "plugin.hpp"

//class callback : public sc_process_b {
//public:
//
//	callback(sc_process_host *host, SC_ENTRY_FUNC func) : sc_process_b(
//			sc_gen_unique_name("callback"),
//			false,
//			true,
//			func,
//			host,
//			0) { }
//	}
//
//};


class top : public sc_module {
public:

	sc_signal<bool>		m_sig;

    top(const sc_module_name &name) : sc_module(name),
    		m_sig("m_sig") {
    }

    void end_of_elaboration() {
        fprintf(stdout, "top::end_of_elaboration sc_argc=%d\n",
			sc_argc());
    }
};



int sc_main(int argc, char **argv) {
  top *t = new top("abc");

  sc_start(sc_time(10, SC_NS));

  return 0;
}

