#define BOOST_FILESYSTEM_NO_DEPRECATED

#include "Reporter.hh"
#include <boost/filesystem.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace TSP__PortType {

Reporter::Reporter(const char *par_port_name)
	: Reporter_BASE(par_port_name)
{
    debug = true;
    output_dir = "/home/dark/report/";
}

Reporter::~Reporter()
{

}

void Reporter::set_parameter(const char * parameter_name,
        const char * parameter_value)
{
    std::string name(parameter_name);
    std::string value(parameter_value);

    if(name == "output_dir") {
        output_dir = value;
    } else if(name == "debug") {
        if(value == "true") {
            debug = true;
        } else {
            debug = false;
        }
    }
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
    // Create current time as a string
    auto time = std::time(nullptr);
    auto tm = *std::localtime(&time);
    std::ostringstream os;
    os << std::put_time(&tm, "%Y-%m-%d-%H-%M-%S");

    subdirectory = std::string(os.str());

    full_path = output_dir / subdirectory;

    boost::system::error_code error_code;
    boost::filesystem::create_directories(full_path, error_code);

    if(error_code.value() != boost::system::errc::success) {
        TTCN_error("Could not create directory %s, error code: %d", full_path.c_str(), error_code.value());
    }
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
    save_metric(send_par);
    save_monitor_metrics(send_par);
}

void Reporter::save_metric(const TSP__Types::Save__Metric& send_par) 
{
    std::ofstream csvfile;
    boost::filesystem::path filename((const char*)send_par.experiment__name());
    boost::filesystem::path csvfile_path = full_path / filename;
    log("Csvfile path: %s", full_path.c_str());
    csvfile.open(csvfile_path.string(), std::ios_base::app);

    if(csvfile.fail()) {
        TTCN_error("Could not open CSV file. Path: %s", csvfile_path.c_str());
    }

    int run = (const int)send_par.run();

    auto parameters = ((const TSP__Types::ParameterConfigurations)send_par.paramcfgs());
    auto metrics = ((const TSP__Types::Metrics)send_par.metrics());

    // Create header
    if(boost::filesystem::file_size(csvfile_path) == 0) {
        csvfile << "run";
        for(int i = 0; i < parameters.size_of(); i++) {
            std::string function_id((const char*)parameters[i].function__id());
            std::string parameter_name((const char*)parameters[i].parameter__name());
            csvfile << "," << parameter_name + ":" + function_id;
        }

        for(int i = 0; i < metrics.size_of(); i++) {
            csvfile << "," << metrics[i].output__parser();
        }
        csvfile << std::endl;
    }

    csvfile << run;

    // Parameter config
    for(int i = 0; i < parameters.size_of(); i++) {
	std::string input((const char*)parameters[i].current__value());
	csvfile << "," << input;
    }

    // and at last the metrics
    for(int i = 0; i < metrics.size_of(); i++) {
        csvfile << "," << metrics[i].metric__value();
    }
    csvfile << std::endl;

    csvfile.close();
}

void Reporter::save_monitor_metrics(const TSP__Types::Save__Metric& send_par) {
    boost::filesystem::path filename((const char*)send_par.experiment__name());
    int run = (const int)send_par.run();
    auto monitor_metrics = (const TSP__Types::Monitor__Metrics)send_par.monitor__metrics();

    for(int i = 0; i < monitor_metrics.size_of(); i++) {
        std::string vnf_name(monitor_metrics[i].vnf__name());
        int interval = (const int)monitor_metrics[i].interval();

        monitor_list_to_csv(monitor_metrics[i].cpu__utilization__list(), "cpu-utilization", vnf_name, run, interval);
        monitor_list_to_csv(monitor_metrics[i].memory__current__list(), "memory-current", vnf_name, run, interval);
        monitor_list_to_csv(monitor_metrics[i].memory__maximum__list(), "memory-maximum", vnf_name, run, interval);
    }

}

template <typename tsp_types_list>
void Reporter::monitor_list_to_csv(tsp_types_list list, std::string metric_name, std::string vnf_name, int run, int intervall) {
    std::ofstream csvfile;
    std::string filename = vnf_name + ":" + metric_name;
    boost::filesystem::path csv_path = full_path / filename;

    csvfile.open(csv_path.string(), std::ios_base::app);
    if(csvfile.fail()) {
        TTCN_error("Could not open csv file");
    }

    if(boost::filesystem::file_size(csv_path) == 0) {
        csvfile << "run" << "," << "monitor_metric" << "," << "time" << std::endl;
    }

    for(int j = 0; j < list.size_of(); j++) {
        csvfile << run << "," << list[j] << "," << j*intervall << std::endl;
    }

    csvfile.close();
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

