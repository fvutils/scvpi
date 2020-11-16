
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

class sub : public sc_module {
public:
    sc_signal<int>              m_sig2;

    sub(const sc_module_name &name) : sc_module(name), m_sig2("m_sig2") {
    }
};

class top : public sc_module {
public:

    sc_signal<bool>		m_sig;
    sub                         m_sub1;
    sub                         m_sub2;

    top(const sc_module_name &name) : sc_module(name),
    		m_sig("m_sig"), m_sub1("m_sub1"), m_sub2("m_sub2") {
    }

    void end_of_elaboration() {
        fprintf(stdout, "top::end_of_elaboration sc_argc=%d\n",
			sc_argc());
    }
};



int sc_main(int argc, char **argv) {
  top *t = new top("abc");

  sc_start(sc_time(10, SC_MS));

  return 0;
}

