#include "ManoMsg.hh"

namespace TSP__PortType {

ManoMsg::ManoMsg(const char *par_port_name)
    : ManoMsg_BASE(par_port_name)
{
}

ManoMsg::~ManoMsg()
{

}

void ManoMsg::set_parameter(const char * parameter_name,
        const char * parameter_value)
{

}

/*void ManoMsg::Handle_Fd_Event(int fd, boolean is_readable,
  boolean is_writable, boolean is_error) {}*/

void ManoMsg::Handle_Fd_Event_Error(int /*fd*/)
{

}

void ManoMsg::Handle_Fd_Event_Writable(int /*fd*/)
{

}

void ManoMsg::Handle_Fd_Event_Readable(int /*fd*/)
{

}

/*void ManoMsg::Handle_Timeout(double time_since_last_call) {}*/

void ManoMsg::user_map(const char * /*system_port*/)
{
}

void ManoMsg::user_unmap(const char * /*system_port*/)
{
}

void ManoMsg::user_start()
{

}

void ManoMsg::user_stop()
{

}

void ManoMsg::outgoing_send(const TSP__Types::Setup__SFC& send_par)
{
	send_successful_operation_status();
}

void ManoMsg::outgoing_send(const TSP__Types::Add__Agents& send_par)
{
	send_successful_operation_status();
}

void ManoMsg::outgoing_send(const TSP__Types::Cleanup__Request& /*send_par*/)
{
	send_successful_operation_status();
}

void ManoMsg::outgoing_send(const TSP__Types::Start__CMD& send_par)
{
    TSP__Types::Start__CMD__Reply cmd_reply;
    TSP__Types::Metrics metrics;
    TSP__Types::Metric metric;
    TSP__Types::Monitor__Metrics monitor_metrics;
    TSP__Types::Monitor__Metric monitor_metric;

    monitor_metric.vnf__name() = CHARSTRING("dummy-vnf");
    monitor_metric.cpu__utilization__list()[0] = CHARSTRING("123456");
    monitor_metric.memory__current__list()[0] = CHARSTRING("123456");
    monitor_metric.memory__maximum__list()[0] = CHARSTRING("123456");
    monitor_metric.interval() = 1;
    monitor_metrics[0] = monitor_metric;

    metric.output__parser() = CHARSTRING("dummy-parser");
    metric.measured__value() = CHARSTRING("123456");

    metrics[0] = metric;

    cmd_reply.metrics() = metrics;
    cmd_reply.monitor__metrics() = monitor_metrics;
    incoming_message(cmd_reply);
}

void ManoMsg::outgoing_send(const TSP__Types::Set__Parameter__Config& send_par)
{
    send_successful_operation_status();
}

void ManoMsg::outgoing_send(const TSP__Types::Add__Monitors& send_par) {
    send_successful_operation_status();
}

void ManoMsg::send_successful_operation_status() {
    TSP__Types::Operation__Status operation_status;
    operation_status.success() = BOOLEAN(true);
    operation_status.reason() = CHARSTRING("");
    incoming_message(operation_status);
}

} /* end of namespace */
