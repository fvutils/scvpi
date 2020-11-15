#pragma once
#include <systemc.h>

using namespace sc_core;

class plugin;

class timed_callback : public sc_process_b {
public:
	timed_callback(
			SC_ENTRY_FUNC		func,
			sc_process_host		*host,
			const sc_time		&t) : sc_process_b(
					0,
					false,
					false,
					func, host, 0) {
		add_static_event(m_ev);
		m_ev.notify(t);
	}

    virtual void disable_process(
        sc_descendant_inclusion_info descendants = SC_NO_DESCENDANTS ) {
    	fprintf(stdout, "disable_process\n");
    }

    virtual void enable_process(
        sc_descendant_inclusion_info descendants = SC_NO_DESCENDANTS ) {
    	fprintf(stdout, "enable_process\n");
    }

    virtual void kill_process(
        sc_descendant_inclusion_info descendants = SC_NO_DESCENDANTS ) {
    	fprintf(stdout, "kill_process\n");
    }

    virtual void resume_process(
        sc_descendant_inclusion_info descendants = SC_NO_DESCENDANTS ) {
    	fprintf(stdout, "resume_process\n");
    }

    virtual void suspend_process(
        sc_descendant_inclusion_info descendants = SC_NO_DESCENDANTS ) {
    	fprintf(stdout, "suspend_process\n");
    }

    virtual void throw_user( const sc_throw_it_helper& helper,
        sc_descendant_inclusion_info descendants = SC_NO_DESCENDANTS ) {
    	fprintf(stdout, "throw_user\n");
    }

    virtual void throw_reset( bool async ) {
    	fprintf(stdout, "throw_reset\n");
    }

private:
	sc_event				m_ev;
};

class callback_host : public sc_process_host {
public:

	callback_host(const sc_time &t) : m_count(0), m_time(t) {
        ::sc_core::sc_process_handle handle =
            sc_core::sc_get_curr_simcontext()->create_method_process(
                "foo",  true, SC_MAKE_FUNC_PTR(callback_host,callback),
                this, 0);
//        handle << m_ev;
	}

	void callback() {
		fprintf(stdout, "callback: count=%d\n", m_count);
		if (m_count == 0) {
			next_trigger(m_time, sc_get_curr_simcontext());
//			notify(m_time, m_ev);
		}
		m_count++;
	}


private:
	uint32_t				m_count;
	sc_event				m_ev;
	sc_time					m_time;
};

class plugin : public sc_module {
public:
    plugin(const sc_module_name &name) : sc_module(name) {
        fprintf(stdout, "plugin\n");
    }

    SC_HAS_PROCESS(plugin);

    void end_of_elaboration() {
        fprintf(stdout, "end_of_elaboration\n");
        for (uint32_t i=1; i<sc_argc(); i++) {
            const char *arg = sc_argv()[i];
            fprintf(stdout, "arg=%s\n", arg);
        }
//        ::sc_core::sc_process_handle handle =
//            sc_core::sc_get_curr_simcontext()->create_method_process(
//                "foo",  false, SC_MAKE_FUNC_PTR(plugin,callback),
//                this, 0);
//        timed_callback *cb = new timed_callback(
//        		SC_MAKE_FUNC_PTR(plugin, callback),
//				this,
//				sc_time(1, SC_NS));
        callback_host *h = new callback_host(sc_time(1, SC_NS));
    }




    void callback() {
    	fprintf(stdout, "callback\n");
    }

};

static plugin p("__plugin");

