#include "Reporter.hh"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

namespace TSP__PortType {

Reporter::Reporter(const char *par_port_name)
	: Reporter_BASE(par_port_name)
{
    debug = true;
    output_dir = "/home/dark/report/";
    header = false;
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
    std::ofstream csvfile;
    std::string filename((const char*)send_par.experiment__name());
    csvfile.open(output_dir + filename, std::ios_base::app);

    int run = (const int)send_par.run();

    auto parameters = ((const TSP__Types::ParameterConfigurations)send_par.paramcfgs());
    TSP__Types::ParameterValues additional_parameters;

    // Create header
    if(!header) {
        for(int i = 0; i < parameters.size_of(); i++) {
            std::string name((const char*)parameters[i].function__id());
            csvfile << "run" << "," << "vcpus:"+name << "," << "memory:" + name << "," << "storage:" + name;
            if(parameters[i].additional__parameters().ispresent()) {
                additional_parameters = (const TSP__Types::ParameterValues)parameters[i].additional__parameters();
                for(int j = 0; j < additional_parameters.size_of() ; j++) {
                    csvfile << "," << std::string(additional_parameters[j].name()) + ":" + name;
                }
            }
            csvfile << "," << "metric" << std::endl;

            log("This should be in the log once!");
            header = true;

        }
    }

    // All main values that vim-emu supports
    int vcpus = (const int)parameters[0].vcpus();
    int memory = (const int)parameters[0].memory();
    int storage = (const int)parameters[0].storage();

    csvfile << run << "," << vcpus << "," << memory << "," << storage;

    // Additional parameter configuration values
    for(int i = 0; i < parameters.size_of(); i++) {
        if(parameters[i].additional__parameters().ispresent()) {
            additional_parameters = (const TSP__Types::ParameterValues)parameters[i].additional__parameters();
            for(int j = 0; j < additional_parameters.size_of() ; j++) {
                csvfile << "," << additional_parameters[j].input();
            }
        }
    }

    // and at last the metric
    std::string metric((const char*)send_par.metric());
    csvfile << "," << metric << std::endl;

    csvfile.close();
}

void Reporter::outgoing_send(const TSP__Types::Save__Monitor__Metric& send_par) {
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

