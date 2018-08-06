#include "Reporter.hh"
#include <string>
#include <vector>
#include <iostream>
#include <elasticlient/client.h>
#include <cpr/response.h>
#include <cpprest/json.h>

// for JSON
using namespace web;

namespace TSP__PortType {

Reporter::Reporter(const char *par_port_name)
	: Reporter_BASE(par_port_name)
{
	elasticsearch_urls = {"http://localhost:9200/"};
    debug = true;
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

void Reporter::outgoing_send(const TSP__Types::Save__Metric& send_par)
{
	elasticlient::Client client(elasticsearch_urls);
    json::value document;
	document["experiment_name"] = json::value::string((const char*)send_par.experiment__name());

	auto parameters = ((const TSP__Types::ParameterConfigurations)send_par.paramcfgs());
	/*for(int i = 0; i < parameters.lengthof(); i++) {
        document["sp_parameters"][i]["vnf_name"] = json::value::string((const char*)parameters[i].function__id());

		// All main values that vim-emu supports
		document["sp_parameters"][i]["vcpus"] = json::value::number((const int)parameters[i].vcpus());
		document["sp_parameters"][i]["memory"] = json::value::number((const int)parameters[i].memory());
        document["sp_parameters"][i]["storage"] = json::value::number((const int)parameters[i].storage());

		// Additional parameter configuration values
		//auto rvalues = (const TSP__Types::RessourceValues)send_par.resourcecfg().resource__values();
		//for(int i = 0; i < rvalues.size_of() ; i++) {
		//    if(rvalues[i].name() == "vcpu") {
		//        vcpus = rvalues[i].actual__value();
		//    } else if(rvalues[i].name() == "memory") {
		//        memory = rvalues[i].actual__value();
		//    }
		//}
	}*/

    //log("Document: ", document.serialize().c_str());
    //cpr::Response indexResponse = client.index("serviceprofiling", "sp_report", "2233", document.serialize().c_str());
}

// TODO: Place logging function to a helper class
void Reporter::log(const char *fmt, ...) {
	if (debug) {
		va_list ap;
		va_start(ap, fmt);
		TTCN_Logger::begin_event(TTCN_DEBUG);
		TTCN_Logger::log_event("ManoMsg Test Port (%s): ", get_name());
		TTCN_Logger::log_event_va_list(fmt, ap);
		TTCN_Logger::end_event();
		va_end(ap);
	}
}


} /* end of namespace */

