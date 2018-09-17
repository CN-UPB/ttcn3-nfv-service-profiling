#include "ManoMsg.hh"
#include "OutputParser.hh"
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include <streambuf>
#include <cstdlib>
#include <boost/algorithm/string.hpp>
#include <boost/process.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <chrono>
#include <thread>
#include <regex>
#include <future>
#include <algorithm>


using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency::streams;

namespace TSP__PortType {

ManoMsg::ManoMsg(const char *par_port_name)
    : ManoMsg_BASE(par_port_name)
{
    debug = true;

    nsd_path = "/home/dark/teaspoon/external/son-examples/service-projects/";

    docker_rest_url = "http://127.0.0.1:2376";
    gatekeeper_rest_url = "http://172.17.0.2:5000";
    vimemu_rest_url = "http://172.17.0.2:5001";
    rest_username = "admin";
    rest_password = "admin";
}

ManoMsg::~ManoMsg()
{

}

/**
 * Handle setting parameters from config file
 * @param parameter_name The name of the parameter
 * @param parameter_value The value of the parameter
 */
void ManoMsg::set_parameter(const char * parameter_name,
        const char * parameter_value)
{
    std::string name(parameter_name);
    std::string value(parameter_value);

    if(name == "nsd_path") {
        nsd_path = value;
    } else if(name == "docker_rest_url") {
        docker_rest_url = value;
    } else if(name == "gatekeeper_rest_url") {
        gatekeeper_rest_url = value;
    } else if(name == "vimemu_rest_url") {
        vimemu_rest_url = value;
    } else if(name == "rest_username") {
        rest_username = value;
    } else if(name == "rest_password") {
        rest_password = value;
    } else if(name == "debug") {
        if(value == "true") {
            debug = true;
        } else {
            debug = false;
        }
    }
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

/**
 * TTCN-3 map operation. Starts vim-emu and waits until it is operational
 */
void ManoMsg::user_map(const char * /*system_port*/)
{
    start_docker_container();
}

/**
 * TTCN-3 unmap operation. Cleans up everything, e.g. stops all Agent VNFs, stops the
 * SFC and stop the vim-emu container
 */
void ManoMsg::user_unmap(const char * /*system_port*/)
{
    stop_all_agents();

    if(!sfc_service_instance_uuid.empty() && !sfc_service_uuid.empty()) {
        stop_sfc_service(sfc_service_uuid, sfc_service_instance_uuid);
    }

    stop_docker_container();
}

void ManoMsg::user_start()
{

}

void ManoMsg::user_stop()
{

}

/**
 * Handles a send operation for a Setup_SFC request, e.g. start the SFC we want to profile
 * @param send_par Setup_SFC request containing the service name and the filepath to service package
 */
void ManoMsg::outgoing_send(const TSP__Types::Setup__SFC& send_par)
{
    if(!sfc_service_instance_uuid.empty() && !sfc_service_uuid.empty()) {
        stop_sfc_service(sfc_service_uuid, sfc_service_instance_uuid);
    }

    std::string service_name = std::string(((const char*)send_par.service__name()));
    std::string filepath = nsd_path + service_name + ".son";

    log("Create SFC from %s", filepath.c_str());

    sfc_service_uuid = upload_package(filepath);
    if(sfc_service_uuid == "-1") {
        send_unsuccessful_operation_status("Could not upload package");
        return;
    }

    sfc_service_instance_uuid = start_sfc_service(sfc_service_uuid);
    if(sfc_service_instance_uuid == "-1") {
        send_unsuccessful_operation_status("Could not instantiate SFC from package");
        return;
    }

    // We have to wait a moment so that the SFC is available
    std::this_thread::sleep_for(std::chrono::seconds(2));
    if(!apply_additional_parameters_for_sfc()) {
        send_unsuccessful_operation_status("Could not set additional parameters");
        return;
    }

    log("SFC created and running");
    send_successful_operation_status();
}

/**
 * Handles a send operation for an Add_Agents request, e.g. start the Agents
 * @param send_par Add_Agents request containing at least one vnf_name, connection point and image
 */
void ManoMsg::outgoing_send(const TSP__Types::Add__Agents& send_par)
{
    auto agents = (const TSP__Types::Agents)send_par.agents();
    for(int i = 0; i < agents.size_of(); i++) {
        std::string vnf_name = std::string(((const char*)agents[i].name()));
        std::string vnf_cp = std::string(((const char*)agents[i].connection__point()));
        std::string vnf_image = std::string(((const char*)agents[i].image()));

        log("Setting up Agent %s with connection point %s from image %s", vnf_name.c_str(), vnf_cp.c_str(), vnf_image.c_str());

        if(!start_agent(vnf_name, vnf_image)) {
            send_unsuccessful_operation_status("Could not start Agent");
            return;
        }

        if(!connect_agent_to_sfc(vnf_name, vnf_cp)) {
            send_unsuccessful_operation_status("Could not connect Agent to SFC");
            return;
        }
    }
    send_successful_operation_status();
}

/**
 * Handles a Cleanup_Request. Stops all VNFs.
 */
void ManoMsg::outgoing_send(const TSP__Types::Cleanup__Request& /*send_par*/)
{
    log("Cleaning up vim-emu!");

    try {
        stop_all_agents();
        stop_docker_container();
        start_docker_container();
    } catch(const std::exception& e) {
        send_unsuccessful_operation_status("Could not clean up");
        return;
    }

    log("Cleaned up vim-emu. New vim-emu instance is running!");
    send_successful_operation_status();
}

/**
 * Handles the send operation for Start_CMD requests, e.g. starts a command in a VNF
 * @param send_par Start_CMD Request containing the VNF, command and an optional output parser
 */
void ManoMsg::outgoing_send(const TSP__Types::Start__CMD& send_par)
{
    log("Started to handle the Start_CMD request");
    auto agents = (const TSP__Types::Agents)send_par.agents();

    // Start the command for each Agent
    for(int i = 0; i < agents.size_of(); i++) {
        std::string vnf_name = std::string(((const char*)agents[i].name()));

        OPTIONAL<CHARSTRING> cmd_optional = ((const OPTIONAL<CHARSTRING>)agents[i].cmd());
        if(!cmd_optional.is_present()) {
            // We dont have to do anything as the Agent does not have any defined commands
            break;
        }
        std::string cmd(cmd_optional());

        OPTIONAL<TSP__Types::charlist> output_parsers_optional = ((const OPTIONAL<TSP__Types::charlist>)agents[i].output__parsers());
        TSP__Types::charlist output_parsers;

        if(output_parsers_optional.is_present()) {
            output_parsers = output_parsers_optional();
            TSP__Types::charlist bla = output_parsers_optional();
        }

        // replace macros in commands
        std::smatch match;
        std::regex IP4("\\$\\{VNF:IP4:(.*)\\}");

        // IP4
        if(std::regex_search(cmd, match, IP4)) {
            log("Replacing IP4 in CMD");
            std::string ip4address;
            std::ssub_match sub_match = match[1];
            std::string vnf_name = sub_match.str();

            http_client client(vimemu_rest_url);
            auto query = uri_builder("/restapi/compute").to_string();

            http_request req(methods::GET);
            req.set_request_uri(query);

            try {
                http_response response = client.request(req).get();

                log("Status Code is: %d", response.status_code());

                if(response.status_code() == status_codes::OK) {
                    // Get IP address and save it
                    auto json_reply = response.extract_json().get();
                    log("JSON Reply: %s", json_reply.serialize().c_str());
                    if(json_reply.is_array()) {
                        for(auto const all_elements : json_reply.as_array()) {
                            std::string json_element_name("\"" + vnf_name + "\"");
                            if(all_elements.at(0).serialize() == json_element_name ) {
                                auto network = all_elements.at(1).at("network");

                                for(size_t i = 0; i < network.size(); i++) {
                                    if(network.at(i).at("intf_name").serialize() == "\"input\"") {
                                        std::string ip_cidr = network.at(i).at("ip").serialize();
                                        if(ip_cidr.size() > 2) {
                                            ip_cidr.erase(0,1);
                                            ip_cidr.erase(ip_cidr.size() - 1);

                                            std::vector<std::string> ip_cidr_split;
                                            boost::split(ip_cidr_split, ip_cidr, boost::is_any_of("/"));
                                            ip4address = ip_cidr_split[0];
                                        }
                                    }
                                }
                            }
                        }
                    }

                    //auto ip_address = json_reply.at("")[0].at("ip").as_string();
                    //std::vector<std::string> ip_address_elements;
                    //boost::split(ip_address_elements, ip_address, boost::is_any_of("/"));
                    //ip_agents[vnf_name] = ip_address_elements[0];


                } else {
                    send_unsuccessful_operation_status("Could not start CMD, as the IP address could not be determined");
                    return;
                }
            } catch (const http_exception &e) {
                    send_unsuccessful_operation_status("Could not start CMD, as the IP address could not be determined (Exception)");
                    return;
            }

            cmd = regex_replace(cmd, IP4, ip4address);
            log("Replaced IP4 in command. IP4 is: %s", ip4address.c_str());
        }

        std::string ssh_start_cmd = "docker exec mn." + vnf_name + " service ssh start";
        try {
            start_local_program(ssh_start_cmd);
        } catch(const std::exception& e) {
            send_unsuccessful_operation_status("Could not start SSH");
            return;
        }

        std::string ssh_get_port_cmd = "docker port mn." + vnf_name + " 22";
        std::string port_stdout;
        try {
            port_stdout = start_local_program(ssh_get_port_cmd);
        } catch(const std::exception& e) {
            send_unsuccessful_operation_status("Could not get SSH port");
            return;
        }

        // Split port from command output
        std::vector<std::string> port_elements;
        boost::split(port_elements, port_stdout, boost::is_any_of(":"));
        std::string agent_port = port_elements[1];

        // TODO: Configureable password
        std::string command = "sshpass -p \"root\" ssh -oStrictHostKeyChecking=no root@localhost -p " + agent_port + " " + cmd;

        if(output_parsers.size_of() == 0) {
            try {
                start_local_program(command, true);
            } catch(const std::exception& e) {
                send_unsuccessful_operation_status("Could not start program");
                return;
            }
        } else {
            // Get the command output
            std::string command_stdout;
            try {
                command_stdout = start_local_program(command);
            } catch(const std::exception& e) {
                send_unsuccessful_operation_status("Could not start program");
                return;
            }

            log("Command output: %s", command_stdout.c_str());
            std::map<std::string, std::string> agent_metrics;

            for(int parser_index = 0; parser_index < output_parsers.size_of(); parser_index++) {
                std::string output_parser(output_parsers[parser_index]);
                try {
                    agent_metrics[output_parser] = OutputParser::parse(command_stdout, output_parser);
                } catch(const std::exception& e) {
                    send_unsuccessful_operation_status("Could not parse output");
                    return;
                }
                log("Collected metric: %s", agent_metrics[output_parser].c_str());
            }

            log("Stopping Monitors");
            // Stop all monitors
            for(const auto & monitor : monitor_objects) {
                monitor->stop();
                delete monitor;
            }
            monitor_objects.clear();

            log("Stopped Monitors");

            // Construct the reply
            TSP__Types::Monitor__Metrics monitor_metrics;
            for(auto & future : monitor_futures) {
                try {
                    std::map<std::string, std::map<std::string, std::vector<std::string>>> metrics_vnfs = future.get();
                    for(auto metric_vnf : metrics_vnfs ) {
                        TSP__Types::Monitor__Metric monitor_metric;
                        monitor_metric.vnf__name() = CHARSTRING(metric_vnf.first.c_str());

                        for(auto & metric_specifier : metric_vnf.second) {
                            if(metric_specifier.first == "interval") {
                                for(auto & interval : metric_specifier.second) {
                                    monitor_metric.interval() = std::atoi(interval.c_str());
                                    break;
                                }
                            } else if(metric_specifier.first == "cpu-utilization") {
                                for(auto & metric : metric_specifier.second) {
                                    CHARSTRING metric_value(metric.c_str());
                                    int index_next_element;
                                    if(monitor_metric.cpu__utilization__list().is_bound()) {
                                        index_next_element = monitor_metric.cpu__utilization__list().size_of();
                                        monitor_metric.cpu__utilization__list()[index_next_element] = metric_value;
                                    } else {
                                        index_next_element = 0;
                                    }
                                    monitor_metric.cpu__utilization__list()[index_next_element] = metric_value;
                                }
                            } else if(metric_specifier.first == "memory-maximum") {
                                for(auto & metric : metric_specifier.second) {
                                    CHARSTRING metric_value(metric.c_str());
                                    int index_next_element;
                                    if(monitor_metric.memory__maximum__list().is_bound()) {
                                        index_next_element = monitor_metric.memory__maximum__list().size_of();
                                        monitor_metric.memory__maximum__list()[index_next_element] = metric_value;
                                    } else {
                                        index_next_element = 0;
                                    }
                                    monitor_metric.memory__maximum__list()[index_next_element] = metric_value;
                                }
                            } else if(metric_specifier.first == "memory-current") {
                                for(auto & metric : metric_specifier.second) {
                                    CHARSTRING metric_value(metric.c_str());
                                    int index_next_element;
                                    if(monitor_metric.memory__current__list().is_bound()) {
                                        index_next_element = monitor_metric.memory__current__list().size_of();
                                        monitor_metric.memory__current__list()[index_next_element] = metric_value;
                                    } else {
                                        index_next_element = 0;
                                    }
                                    monitor_metric.memory__current__list()[index_next_element] = metric_value;
                                }
                            }
                        }

                        int index_mm_next_element = monitor_metrics.is_bound() ? monitor_metrics.size_of() : 0;
                        monitor_metrics[index_mm_next_element] = monitor_metric;
                    }
                } catch(const std::exception& e) {
                    log("Something with the future was wrong: %s, valid: %d", e.what(), future.valid());
                    send_unsuccessful_operation_status("Could not get future");
                    return;
                }
            }
            monitor_futures.clear();

            TSP__Types::Start__CMD__Reply cmd_reply;
            TSP__Types::Metrics metrics;

            for(auto agent_metric : agent_metrics) {
                TSP__Types::Metric metric;

                metric.output__parser() = CHARSTRING(agent_metric.first.c_str());
                metric.metric__value() = CHARSTRING(agent_metric.second.c_str());

                int index_next_element = metrics.is_bound() ? metrics.size_of() : 0;
                metrics[index_next_element] = metric;
            }

            cmd_reply.metrics() = metrics;
            if(monitor_metrics.is_bound()) {
                cmd_reply.monitor__metrics() = monitor_metrics;
            }

            send_successful_operation_status();
            incoming_message(cmd_reply);
        }
    }

    log("Handled the Start_CMD Request");
}

/**
 * Handles the send operation for Set_Parameter_Config requests
 * @param send_par Set_Parameter_Config request (e.g. service name and parameter configs)
 */
void ManoMsg::outgoing_send(const TSP__Types::Set__Parameter__Config& send_par)
{
    std::string service_name = std::string(((const char*)send_par.service__name()));
    auto parameters = ((const TSP__Types::ParameterConfigurations)send_par.paramcfg());

    std::map<std::string, std::map<std::string, std::string>> vnf_to_parameter_dict;

    for(int i = 0; i < parameters.lengthof(); i++) {
        std::string vnf_name = (const char*)parameters[i].function__id();
        std::string additional_parameter_name((const char*)parameters[i].parameter__name());
        std::string additional_parameter_value((const char*)parameters[i].current__value());

        if(additional_parameter_name == "vcpus" || additional_parameter_name == "memory" || additional_parameter_name == "storage") {
            vnf_to_parameter_dict[vnf_name][additional_parameter_name] = additional_parameter_value;
        } else {
            if(additional_parameter_name == "cpu_time") {
                additional_parameter_name = "cpu_bw";
            } else if(additional_parameter_name == "numa_nodes") {
                additional_parameter_name = "cpuset_mems";
            } else if(additional_parameter_name == "cpu_set") {
                additional_parameter_name = "cpuset_cpus";
            }
            // We handle additional parameters at another location
            vnf_additional_parameters[vnf_name][additional_parameter_name] = additional_parameter_value;
        }
    }

    // We have to replace _ with - because of file structure
    //std::replace(vnf_name.begin(), vnf_name.end(), '_', '-');


    for(auto const& vnf_param : vnf_to_parameter_dict) {
        std::string vnf_name(vnf_param.first);
        log("Setting parameter configuration of VNF %s", vnf_name.c_str());

        std::string vcpus(vnf_to_parameter_dict[vnf_name]["vcpus"]);
        std::string memory(vnf_to_parameter_dict[vnf_name]["memory"]);
        std::string storage(vnf_to_parameter_dict[vnf_name]["storage"]);

        std::string filename = nsd_path  + service_name + "-emu" + "/sources/vnf/" + vnf_name + "/" + vnf_name + "-vnfd.yml";
        log("Filename %s", filename.c_str());

        // Change the Parameters
        std::string cmd = "python2 ../bin/set-resource-configuration.py " + filename + " " + vnf_name + " " + vcpus + " " + memory + " " + storage;
        log("Command: %s", cmd.c_str());

        try {
            start_local_program(cmd);
        } catch(const std::exception& e) {
            send_unsuccessful_operation_status("Could not set parameters with python script");
            return;
        }
    }

    // Create the service package
    std::string son_cmd = "son-package --project " + nsd_path  + service_name + "-emu -d " + nsd_path + " -n " + service_name;
    log("Command: %s", son_cmd.c_str());
    std::string son_cmd_output;
    try {
        son_cmd_output = start_local_program(son_cmd);
        log("Command output: %s", son_cmd_output.c_str());

        log("Setting parameter configuration of SFC %s completed", service_name.c_str());
        send_successful_operation_status();
    } catch (const std::runtime_error& e) {
        log("Error when executing %s", son_cmd.c_str());

        send_unsuccessful_operation_status("Could not run son-package");
    }
}

void ManoMsg::outgoing_send(const TSP__Types::Add__Monitors& send_par) {
    for(int i = 0; i < send_par.monitors().size_of(); i++) {
        std::string vnf_name((const char*)send_par.monitors()[i].vnf__name());
        int interval = ((const int)send_par.monitors()[i].interval());
        auto metrics = (const TSP__Types::charlist)send_par.monitors()[i].metrics();

        std::vector<std::string> metrics_vec;
        for(int n = 0; n < metrics.size_of(); n++) {
            metrics_vec.push_back(std::string(metrics[n]));
        }

        log("Starting Monitor for %s, Interval: %d, Docker API URL: %s", vnf_name.c_str(), interval, docker_rest_url.c_str());
        auto monitor = new ManoMsg::Monitor(vnf_name, metrics_vec, interval, docker_rest_url);

        monitor_objects.push_back(monitor);

        monitor_futures.push_back(std::move(monitor->run()));
    }

    log("Added Monitors");
    send_successful_operation_status();
}

/**
 * Logging function for TITAN (copied from documentation)
 */
void ManoMsg::log(const char *fmt, ...) {
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

/**
 * Uploads a SFC package (packaged as a SONATA package file) to vim-emu
 * @param filepath The filepath to the SFC package
 */
std::string ManoMsg::upload_package(std::string filepath) {
    log("Upload package from %s", filepath.c_str());

    // TODO: test if file exists
    auto fileStream = file_stream<unsigned char>::open_istream(filepath).get();
    fileStream.seek(0, std::ios::end);
    auto length = static_cast<size_t>(fileStream.tell());
    fileStream.seek(0, std::ios::beg);

    auto query =  uri_builder("/packages").to_string();
    http_request req(methods::POST);
    req.set_body(fileStream, length);
    req.set_request_uri(query);

    http_client client(gatekeeper_rest_url);

    try {
        http_response response = client.request(req).get();

        if(response.status_code() == status_codes::Created) {
            auto json_reply = response.extract_json().get();
            auto service_uuid = json_reply.at("service_uuid").as_string();

            log("Uploaded package from %s as service_uuid %s", filepath.c_str(), service_uuid.c_str());

            return service_uuid;
        } else {
            return "-1";
        }
    } catch (const http_exception &e) {
        return "-1";
    }

    return "-1";
}

/**
 * Instantiates the SFC
 * @param service_uuid Service UUID of the SFC
 */
std::string ManoMsg::start_sfc_service(std::string service_uuid) {
    log("Start SFC with uuid %s", service_uuid.c_str());

    json::value postParameters = web::json::value::object();
    postParameters["service_uuid"] = json::value::string(service_uuid);

    http_client client(gatekeeper_rest_url);
    auto query =  uri_builder("/instantiations").to_string();

    http_request req(methods::POST);
    req.set_request_uri(query);
    req.set_body(postParameters);

    try {
        http_response response = client.request(req).get();

        if(response.status_code() == status_codes::Created) {
            auto json_reply = response.extract_json().get();
            auto service_instance_uuid = json_reply.at("service_instance_uuid").as_string();

            log("Started SFC as service_instance_uuid %s", service_instance_uuid.c_str());

            return service_instance_uuid;
        } else {
            return "-1";
        }
    } catch (const http_exception &e) {
        return "-1";
    }
}

/**
 * Stops the SFC
 * @param service_uuid Service UUID of the SFC
 * @param service_instance_uuid Service Instance UUID of the SFC
 */
bool ManoMsg::stop_sfc_service(std::string service_uuid, std::string service_instance_uuid) {
    log("Stop service with uuid %s and instance uuid %s", service_uuid.c_str(), service_instance_uuid.c_str());

    json::value postParameters = web::json::value::object();
    postParameters["service_uuid"] = json::value::string(service_uuid);
    postParameters["service_instance_uuid"] = json::value::string(service_instance_uuid);

    http_client client(gatekeeper_rest_url);
    auto query =  uri_builder("/instantiations").to_string();

    http_request req(methods::DEL);
    req.set_request_uri(query);
    req.set_body(postParameters);

    try {
        http_response response = client.request(req).get();

        if(response.status_code() == status_codes::OK) {
            log("Stopped service");
            return true;
        } else {
            return false;
        }
    } catch (const http_exception &e) {
        return false;
    }
}

/**
 * Applies the additional parameters that were parsed when the Set_Parameter_Config Reply was sent
 */
bool ManoMsg::apply_additional_parameters_for_sfc() {
    log("Setting additional parameters");
    for(const auto& vnf_map : vnf_additional_parameters) {
        std::string vnf_name(vnf_map.first);

        for(const auto& parameter_map : vnf_map.second) {
            std::string parameter_name(parameter_map.first);
            std::string value(parameter_map.second);

            http_client client(vimemu_rest_url);
            auto query_builder = uri_builder("/restapi/compute/resources/dc1/" + vnf_name); // TODO: dc1 configureable
            query_builder.set_query(parameter_name + "=" + value);

            auto query = query_builder.to_string();

            http_request req(methods::PUT);
            req.set_request_uri(query);

            try {
                http_response response = client.request(req).get();

                if(response.status_code() == status_codes::OK) {
                    log("Setting the parameter %s of %s to %s was successful", parameter_name.c_str(), vnf_name.c_str(), value.c_str());
                } else {
                    log("Setting the parameter %s of %s to %s was NOT successful", parameter_name.c_str(), vnf_name.c_str(), value.c_str());
                    return false;
                }
            } catch (const http_exception &e) {
                log("Setting the parameter %s of %s to %s was NOT successful", parameter_name.c_str(), vnf_name.c_str(), value.c_str());
                return false;
            }

        }
    }

    return true;
}

/**
 * Start an Agent
 * @param vnf_name Name of the VNF
 * @param vnf_image Image of the VNF
 * @return True if the operation was successful, else false
 */
bool ManoMsg::start_agent(std::string vnf_name, std::string vnf_image) {
    log("Start Agent VNF with name %s from image %s", vnf_name.c_str(), vnf_image.c_str());

    json::value postParameters = web::json::value::object();
    postParameters["image"] = json::value::string(vnf_image);

    http_client client(vimemu_rest_url);
    auto query = uri_builder("/restapi/compute/dc1/" + vnf_name).to_string(); // TODO: dc1 configureable

    http_request req(methods::PUT);
    req.set_request_uri(query);
    req.set_body(postParameters);

    try {
        http_response response = client.request(req).get();

        if(response.status_code() == status_codes::OK) {
            running_agents.push_back(vnf_name);

            // Get IP address and save it
            //auto json_reply = response.extract_json().get();
            //auto ip_address = json_reply.at("network")[0].at("ip").as_string();
            //std::vector<std::string> ip_address_elements;
            //boost::split(ip_address_elements, ip_address, boost::is_any_of("/"));
            //ip_agents[vnf_name] = ip_address_elements[0];

            log("Agent VNF %s created", vnf_name.c_str());

            return true;
        } else {
            return false;
        }
    } catch (const http_exception &e) {
        return false;
    }
}

/**
 * Stops all Agent VNFs 
 * @return True if the operation was successful, else false
 */
bool ManoMsg::stop_all_agents() {
    log("Stopping all Agents");

    bool successful = true;

    for(std::string agent: running_agents) {
        if(!stop_agent(agent)) {
            successful = false;
        }
    }

    running_agents.clear();

    return successful;
}

/**
 * Stops an Agent
 * @param vnf_name The VNF that should be stopped
 */
bool ManoMsg::stop_agent(std::string vnf_name) {
    log("Stopping Agent %s", vnf_name.c_str());

    http_client client(vimemu_rest_url);
    auto query = uri_builder("/restapi/compute/dc1/" + vnf_name).to_string(); // TODO: dc1 configureable

    http_request req(methods::DEL);
    req.set_request_uri(query);

    try {
        http_response response = client.request(req).get();

        if(response.status_code() == status_codes::OK) {
            //running_vnfs.push_back(vnf_name); TODO: Reverse operation (delete)
            log("Agent VNF %s stopped", vnf_name.c_str());

            return true;
        } else {
            return false;
        }
    } catch (const http_exception &e) {
        return false;
    }
}

/**
 * Connects an Agent to an SFC
 * @param vnf_name The name of the VNF that should be connected to the SFC
 * @param vnf_cp The connection point that the VNF should be connected to
 * @return true if the operation was successful, else false
 */
bool ManoMsg::connect_agent_to_sfc(std::string vnf_name, std::string vnf_cp) {
    log("Connect %s to connection point %s", vnf_name.c_str(), vnf_cp.c_str());


    std::vector<std::string> vnf_cp_elements;
    if(vnf_cp.find(":") != std::string::npos) {
        boost::split(vnf_cp_elements, vnf_cp, boost::is_any_of(":"));
    } else {
        TTCN_error("Connection point %s has the wrong format. \":\" expected", vnf_cp.c_str());
    }

    json::value postParameters = web::json::value::object();
    postParameters["vnf_src_name"] = json::value::string(vnf_name);
    postParameters["vnf_dst_name"] = json::value::string(vnf_cp_elements[0]);
    postParameters["vnf_src_interface"] = json::value::string(vnf_name + "-eth0");
    postParameters["vnf_dst_interface"] = json::value::string(vnf_cp_elements[1]);
    postParameters["bidirectional"] = json::value::string("True");
    postParameters["cookie"] = json::value::string("10");
    postParameters["priority"] = json::value::string("1000");


    http_client client(vimemu_rest_url);
    auto query = uri_builder("/restapi/network").to_string(); // TODO: dc1 configureable

    http_request req(methods::PUT);
    req.set_request_uri(query);
    req.set_body(postParameters);

    try {
        http_response response = client.request(req).get();

        if(response.status_code() == status_codes::OK) {
            log("Connected to connection point");

            return true;
        } else {
            return false;
        }
    } catch (const http_exception &e) {
        return false;
    }
}

/**
 * Start the local vim-emu Docker container
 */
void ManoMsg::start_docker_container() {
    log("Starting docker container");

    std::string cmd = "sh -c \"docker run --name vim-emu -d --rm --privileged --pid='host' -v /var/run/docker.sock:/var/run/docker.sock vim-emu-img\"";
    start_local_program_and_wait(cmd);

    for(int i = 10; i >= 0; i--) {
        if(i == 0) {
            TTCN_error("Could not connect to vim-emu!");
        }

        if(wait_for_vim_emu(30)) {
            break;
        } else {
            stop_docker_container();
            start_local_program_and_wait(cmd);
        }
    }

    log("Started docker container");
}

/**
 * Tries to connect to vim-emu until it is available or the maximum amount of retries have been reached
 * @param retries Number of retries
 * @return True if the connection to vim-emu was successful, else false
 */
bool ManoMsg::wait_for_vim_emu(int retries) {
    // Test connection to vim-emu (and wait until it is available)
    http_client client(gatekeeper_rest_url);
    auto query =  uri_builder("/instantiations").to_string();

    while(retries > 0) {
        try {
            http_response response = client.request(methods::GET, query).get();

            if(response.status_code() == status_codes::OK) {
                return true;
            }
        } catch (const http_exception &e) {
            log("Could connect to vim-emu: %s", e.what());
        }

        log("Retrying request to vim-emu");
        // wait if the request did not work
        std::this_thread::sleep_for(std::chrono::seconds(5));

        retries--;
    }

    return false;
}

/**
 * Stops the vim-emu docker container
 */
void ManoMsg::stop_docker_container() {
    log("Stopping docker container");
    std::string cmd = "sh -c \"docker stop vim-emu\"";

    start_local_program_and_wait(cmd);

    log("Stopped docker container");
}

/**
 * Start a program and waits until is has exited
 * @param command The command that should be executed
 */
void ManoMsg::start_local_program_and_wait(std::string command) {
    log("Starting command \"%s\"", command.c_str());

    std::error_code ec;
    boost::process::system(command,
            boost::process::std_in.close(),
            boost::process::std_out > boost::process::null,
            boost::process::std_err > boost::process::null,
            ec);

    if(ec.value() != boost::system::errc::success) {
        throw std::runtime_error("Could not run program");
    }
}

/**
 * Start a program on the local machine
 * @param command Command that should be started
 * @param background true, if the program should be started in background, else false
 * @return The stdout of the program
 */
std::string ManoMsg::start_local_program(std::string command, bool background) {
    log("Starting command \"%s\"", command.c_str());
    boost::asio::io_service ios;

    std::future<std::string> data;
    std::error_code ec;

    if(background) {
        boost::process::spawn(command,
                boost::process::std_in.close(),
                boost::process::std_out > boost::process::null,
                boost::process::std_err > boost::process::null,
                ec);

        if(ec.value() != boost::system::errc::success) {
            throw std::runtime_error("Could not run the command");
        }
    } else {
        boost::process::child c(command,
                boost::process::std_in.close(),
                boost::process::std_out > data,
                boost::process::std_err > boost::process::null,
                ec,
                ios);


        try {
            ios.run();
        } catch(const boost::process::process_error &e) {
            throw std::runtime_error("Could not run the command");
        }

        if(ec.value() != boost::system::errc::success) {
            throw std::runtime_error("Could not run the command");
        }


        try {
            auto output = data.get();
            if(output.size() > 0) {
                output.pop_back(); // Delete last character, e.g. last newline
            }

            return output;
        } catch (const boost::process::process_error &e) {
            throw std::runtime_error("Could not run the command");
        }
    }

    // The method should never return here
    return "";
}

ManoMsg::Monitor::Monitor(std::string vnf_name, std::vector<std::string> metrics, int interval, std::string docker_rest_url) {
    this->vnf_name = vnf_name;
    this->metrics = metrics;
    this->docker_rest_url = docker_rest_url;
    this->interval = interval;
}

double ManoMsg::Monitor::calculate_cpu_percent(web::json::value json) {
    // The following algorithm is based on the Moby Project:
    // https://github.com/moby/moby/blob/eb131c5383db8cac633919f82abad86c99bffbe5/cli/command/container/stats_helpers.go#L175
    double cpu_utilization = 0.0;

    uint64_t cpu_total = json["cpu_stats"]["cpu_usage"]["total_usage"].as_number().to_uint64();
    uint64_t pre_cpu_total = json["precpu_stats"]["cpu_usage"]["total_usage"].as_number().to_uint64();
    uint64_t cpu_system = json["cpu_stats"]["system_cpu_usage"].as_number().to_uint64();
    uint64_t pre_cpu_system_total =  json["precpu_stats"]["system_cpu_usage"].as_number().to_uint64();
    uint64_t number_of_cpus = json["cpu_stats"]["online_cpus"].as_number().to_uint64();

    double cpu_delta = double(cpu_total) - double(pre_cpu_total);
    double system_delta = double(cpu_system) - double(pre_cpu_system_total);

    if(cpu_total > 0 && system_delta > 0) {
        cpu_utilization = (cpu_delta / system_delta) * number_of_cpus * 100;
    }

    return cpu_utilization;
}

std::future<std::map<std::string, std::map<std::string, std::vector<std::string>>>> ManoMsg::Monitor::run() {
    std::function<std::map<std::string, std::map<std::string, std::vector<std::string>>>(std::string vnf_name, std::vector<std::string> metrics, int interval, std::string docker_rest_url, boolean* running)> collectMonitorMetric = [](std::string vnf_name, std::vector<std::string> metrics, int interval, std::string docker_rest_url, boolean* running) {
        std::map<std::string, std::vector<std::string>> metric_values;
        while(*running) {
            http_client client(docker_rest_url);
            auto query = uri_builder("/containers/mn." + vnf_name + "/stats").set_query("stream=0").to_string();


            http_request req(methods::GET);
            req.set_request_uri(query);

            http_response response = client.request(req).get();

            if(response.status_code() == status_codes::OK) {
                auto json_reply = response.extract_json().get();

                if(std::find(metrics.begin(), metrics.end(), "cpu-utilization") != metrics.end()) {
                    double cpu_percent = ManoMsg::Monitor::calculate_cpu_percent(json_reply);
                    metric_values["cpu-utilization"].push_back(std::to_string(cpu_percent));
                }

                if(std::find(metrics.begin(), metrics.end(), "memory-maximum") != metrics.end()) {
                    uint64_t max_usage = json_reply["memory_stats"]["max_usage"].as_number().to_uint64() / 1024 / 1024;
                    metric_values["memory-maximum"].push_back(std::to_string(max_usage));
                }

                if(std::find(metrics.begin(), metrics.end(), "memory-current") != metrics.end()) {
                    uint64_t usage = json_reply["memory_stats"]["usage"].as_number().to_uint64() / 1024 / 1024;
                    metric_values["memory-current"].push_back(std::to_string(usage));
                }
            } else if(response.status_code() == status_codes::NotFound) {
                // The VNF is not running anymore
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(interval));
        }

        metric_values["interval"].push_back(std::to_string(interval));

        std::map<std::string, std::map<std::string, std::vector<std::string>>> output;
        output[vnf_name] = metric_values;

        return output;
    };

    this->running = true;
    return std::async(std::launch::async, collectMonitorMetric, vnf_name, metrics, interval, docker_rest_url, &running);
}

void ManoMsg::Monitor::stop() {
    this->running = false;
}

void ManoMsg::send_unsuccessful_operation_status(std::string reason) {
    log("Unsuccessful operation: %s", reason.c_str());
    TSP__Types::Operation__Status operation_status;
    operation_status.success() = BOOLEAN(false);
    operation_status.reason() = CHARSTRING(reason.c_str());
    incoming_message(operation_status);
}

void ManoMsg::send_successful_operation_status() {
    TSP__Types::Operation__Status operation_status;
    operation_status.success() = BOOLEAN(true);
    operation_status.reason() = CHARSTRING("");
    incoming_message(operation_status);
}

} /* end of namespace */
