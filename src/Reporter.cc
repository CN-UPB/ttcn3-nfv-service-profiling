#include "Reporter.hh"

namespace TSP__PortType {

Reporter::Reporter(const char *par_port_name)
	: Reporter_BASE(par_port_name)
{

}

Reporter::~Reporter()
{

}

void Reporter::set_parameter(const char * /*parameter_name*/,
	const char * /*parameter_value*/)
{

}

/*void Reporter::Handle_Fd_Event(int fd, boolean is_readable,
	boolean is_writable, boolean is_error) {}*/

void Reporter::Handle_Fd_Event_Error(int /*fd*/)
{

}

void Reporter::Handle_Fd_Event_Writable(int /*fd*/)
{

}

void Reporter::Handle_Fd_Event_Readable(int /*fd*/)
{

}

/*void Reporter::Handle_Timeout(double time_since_last_call) {}*/

void Reporter::user_map(const char * /*system_port*/)
{

}

void Reporter::user_unmap(const char * /*system_port*/)
{

}

void Reporter::user_start()
{

}

void Reporter::user_stop()
{

}

void Reporter::outgoing_send(const TSP__Types::Save__Metric& /*send_par*/)
{

}

} /* end of namespace */

