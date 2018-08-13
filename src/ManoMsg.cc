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
#include <chrono>
#include <thread>
#include <regex>
#include <future>


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
    debug_http = true;

    //vnf_path = "/home/dark/son-examples/service-projects/sonata-empty-service-emu/sources/vnf/";
    nsd_path = "/home/dark/teaspoon/external/son-examples/service-projects/";

    docker_rest_url = "http://127.0.0.1:2376";
    gatekeeper_rest_url = "http://172.17.0.2:5000";
    vimemu_rest_url = "http://172.17.0.2:5001";
    rest_username = "admin";
    rest_password = "admin";

    manage_docker = true;

    //sfc_service_instance_uuid = "";
    //sfc_service_uuid = "";
    
    metric_collecting = false;
}

ManoMsg::~ManoMsg()
{

}

void ManoMsg::set_parameter(const char * /*parameter_name*/,
        const char * /*parameter_value*/)
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

/**
 * TTCN-3 map operation. Starts vim-emu and waits until it is operational
 */
void ManoMsg::user_map(const char * /*system_port*/)
{
    startDockerContainer();

    http_client client(gatekeeper_rest_url);
    auto query =  uri_builder("/instantiations").to_string();

    int retries = 5;
    bool rest_online = false;
    while(!rest_online) {
        try {
            http_response response = client.request(methods::GET, query).get();

            if(response.status_code() == status_codes::OK) {
                rest_online = true;
            }
        } catch (const http_exception &e) {
            log("Could connect to vim-emu: %s", e.what());
        }

        if(retries <= 0) {
            TTCN_error("Could connect to vim-emu!");
        } else if (!rest_online) {
            log("Retrying request");
            // wait 2 seconds if the request did not work
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        }

        retries--;
    }
}

/**
 * TTCN-3 unmap operation. Cleans up everything, e.g. stops all Agent VNFs, stops the
 * SFC and stop the vim-emu container
 */
void ManoMsg::user_unmap(const char * /*system_port*/)
{
    stopAllAgents();

    if(!sfc_service_instance_uuid.empty() && !sfc_service_uuid.empty()) {
        stopSfcService(sfc_service_uuid, sfc_service_instance_uuid);
    }

    stopDockerContainer();
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
        stopSfcService(sfc_service_uuid, sfc_service_instance_uuid);
    }

    std::string service_name = std::string(((const char*)send_par.service__name()));
    std::string filepath = nsd_path + service_name;

    log("Create SFC from %s", filepath.c_str());

    sfc_service_uuid = uploadPackage(filepath);
    sfc_service_instance_uuid = startSfcService(sfc_service_uuid);

    log("SFC created and running");
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

        startAgent(vnf_name, vnf_image);
        connectAgentToSfc(vnf_name, vnf_cp);
    }
}

/**
 * Handles a Cleanup_Request. Stops all VNFs.
 */
void ManoMsg::outgoing_send(const TSP__Types::Cleanup__Request& /*send_par*/)
{
    log("Cleanup vim-emu!");

    stopAllAgents();
    //stopDockerContainer();
    //startDockerContainer();

    log("Cleaned up vim-emu. New vim-emu instance is running!");
}

/**
 * Handles the send operation for Start_CMD requests, e.g. starts a command in a VNF
 * @param send_par Start_CMD Request containing the VNF, command and an optional output parser
 */
void ManoMsg::outgoing_send(const TSP__Types::Start__CMD& send_par)
{
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

        OPTIONAL<CHARSTRING> output_parser_optional = ((const OPTIONAL<CHARSTRING>)agents[i].output__parser());
        std::string output_parser;

        if(output_parser_optional.is_present()) {
            output_parser = output_parser_optional();
        }

        // replace macros in commands
        std::smatch match;
        std::regex IP4("\\$\\{VNF:IP4:(.*)\\}");
        if(std::regex_search(cmd, match, IP4)) {
            std::ssub_match sub_match = match[1];
            std::string ip4_from_name = sub_match.str();

            auto ip4_address = ip_agents[ip4_from_name];
            std::string replacement = "$`" + ip4_address + "$'";
            cmd = regex_replace(cmd, IP4, replacement);
        }

        log("Starting command %s on vnf %s", cmd.c_str(), vnf_name.c_str());

        std::string ssh_start_cmd = "docker exec mn." + vnf_name + " service ssh start";
        start_local_program(ssh_start_cmd);

        std::string ssh_get_port_cmd = "docker port mn." + vnf_name + " 22";
        auto port_stdout = start_local_program(ssh_get_port_cmd);

        // Split port from command output
        std::vector<std::string> port_elements;
        boost::split(port_elements, port_stdout, boost::is_any_of(":"));
        std::string agent_port = port_elements[1];

        // TODO: Configureable password
        std::string command = "sshpass -p \"root\" ssh -oStrictHostKeyChecking=no root@localhost -p " + agent_port + " " + cmd; //+ " " + server_address;

        if(output_parser.empty()) {
            boost::process::spawn(command,
                    boost::process::std_in.close(),
                    boost::process::std_out > boost::process::null,
                    boost::process::std_err > boost::process::null);
        } else {
            auto command_stdout = start_local_program(command);

            auto metric = OutputParser::parse(command_stdout, output_parser);

            for(auto & future : monitor_futures) {
                try {
                    std::map<std::string, std::map<std::string, std::vector<std::string>>> test = future.get();
                    log("Monitor Output: %s", test["snort_vnf"]["cpu_stats"][0].c_str());
                } catch(const std::exception& e) {
                    log("Something with the future was wrong: %s, valid: %d", e.what(), future.valid());
                }
            }
            monitor_futures.clear();

            TSP__Types::Start__CMD__Reply cmd_reply;
            cmd_reply.metric() = CHARSTRING(metric.c_str());
            incoming_message(cmd_reply);
        }
    }
}

/**
 * Handles the send operation for Set_Parameter_Config requests
 * @param send_par Set_Parameter_Config request (e.g. service name and parameter configs)
 */
void ManoMsg::outgoing_send(const TSP__Types::Set__Parameter__Config& send_par)
{
    std::string service_name = std::string(((const char*)send_par.service__name()));
    auto parameters = ((const TSP__Types::ParameterConfigurations)send_par.paramcfg());

    std::vector<std::string> service_name_elements;
    boost::split(service_name_elements, service_name, boost::is_any_of("."));

    for(int i = 0; i < parameters.lengthof(); i++) {
        std::string vnf_name = (const char*)parameters[i].function__id();

        // All main values that vim-emu supports
        int vcpus = (const int)parameters[i].vcpus();
        int memory = (const int)parameters[i].memory();
        int storage = (const int)parameters[i].storage();

        // TODO
        // Additional parameter configuration values
        //auto rvalues = (const TSP__Types::RessourceValues)send_par.resourcecfg().resource__values();
        //for(int i = 0; i < rvalues.size_of() ; i++) {
        //    if(rvalues[i].name() == "vcpu") {
        //        vcpus = rvalues[i].actual__value();
        //    } else if(rvalues[i].name() == "memory") {
        //        memory = rvalues[i].actual__value();
        //    }
        //}

        log("Setting resource configuration of VNF %s", vnf_name.c_str());

        // TODO
        std::string filename = nsd_path  + service_name_elements[0] + "-emu" + "/sources/vnf/" + vnf_name + "/" + vnf_name + "-vnfd.yml";
        //std::string filename = "/home/dark/son-examples/service-projects/sonata-snort-service-emu/sources/vnf/snort-vnf/snort-vnfd.yml";

        log("Filename %s", filename.c_str());

        // Change the Parameters
        std::string cmd = "python3 ../bin/set-resource-configuration.py " + filename + " " + vnf_name + " " + std::to_string(vcpus) + " " + std::to_string(memory) + " " + std::to_string(storage);
        log("Command: %s", cmd.c_str());

        std::system(cmd.c_str());
    }

    // Create the service package
    std::string son_cmd = "son-package --project " + nsd_path  + service_name_elements[0] + "-emu -d " + nsd_path + " -n " + service_name_elements[0];
    log("Command: %s", son_cmd.c_str());
    auto son_cmd_output = start_local_program(son_cmd);

    log("Command output: %s", son_cmd_output.c_str());
    //boost::process::spawn(son_cmd,
    //        boost::process::std_in.close(),
    //        boost::process::std_out > boost::process::null,
    //        boost::process::std_err > boost::process::null);

    log("Setting resource configuration of SFC %s completed", service_name.c_str());
}

void ManoMsg::outgoing_send(const TSP__Types::Add__Monitors& send_par) {
    return; // TODO
    std::string service_name((const char*)send_par.service__name());
    int interval = ((const int)send_par.interval());

    for(int i = 0; i < send_par.monitors().size_of(); i++) {
        std::string vnf_name((const char*)send_par.monitors()[i].vnf__name());
        auto metrics = (const TSP__Types::Metrics)send_par.monitors()[i].metrics();

        for(int n = 0; n < metrics.size_of(); n++) {
            std::string metric(metrics[n]);
            log("Adding Monitor %s with metric %s", vnf_name.c_str(), metric.c_str());

            // Return: [vnf_name, {metric_name, values}]
            std::function<std::map<std::string, std::map<std::string, std::vector<std::string>>>()> collectMonitorMetric = [&]() {
                std::map<std::string, std::vector<std::string>> metric_values;
                http_client client(docker_rest_url);
                auto query = uri_builder("/containers/mn." + vnf_name + "/stats").set_query("stream=0").to_string();

                http_request req(methods::GET);
                req.set_request_uri(query);

                try {
                    http_response response = client.request(req).get();

                    if(response.status_code() == status_codes::OK) {
                        auto json_reply = response.extract_json().get();
                        auto cpu_stats = json_reply.at("cpu_stats");

                        metric_values["cpu_stats"].push_back(cpu_stats.serialize());
                    }
                } catch (const http_exception &e) {
                    // Something is broken and we do not handle exceptions yet
                }

                // Wait for specified interval
                //std::this_thread::sleep_for(std::chrono::seconds(interval));

                std::map<std::string, std::map<std::string, std::vector<std::string>>> output;
                output[vnf_name] = metric_values;

                return output;
            };

            monitor_futures.push_back(std::async(std::launch::async, collectMonitorMetric));
        }
    }

    log("Added Monitors");
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
std::string ManoMsg::uploadPackage(std::string filepath) {
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
            TTCN_error("Could upload package to vim-emu. Status: %d", response.status_code());
        }
    } catch (const http_exception &e) {
        TTCN_error("Could upload package to vim-emu: %s", e.what());
    }
}

/**
 * Instantiates the SFC
 * @param service_uuid Service UUID of the SFC
 */
std::string ManoMsg::startSfcService(std::string service_uuid) {
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
            TTCN_error("Could not start SFC. Status: %d", response.status_code());
        }
    } catch (const http_exception &e) {
        TTCN_error("Could not start SFC: %s", e.what());
    }
}

/**
 * Stops the SFC
 * @param service_uuid Service UUID of the SFC
 * @param service_instance_uuid Service Instance UUID of the SFC
 */
void ManoMsg::stopSfcService(std::string service_uuid, std::string service_instance_uuid) {
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
            return;
        } else {
            TTCN_error("Could not stop SFC. Status: %d", response.status_code());
        }
    } catch (const http_exception &e) {
        TTCN_error("Could not stop SFC: %s", e.what());
    }

    log("Stopped service");
}

/**
 * Start an Agent
 * @param vnf_name Name of the VNF
 * @param vnf_image Image of the VNF
 */
void ManoMsg::startAgent(std::string vnf_name, std::string vnf_image) {
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
            auto json_reply = response.extract_json().get();
            auto ip_address = json_reply.at("network")[0].at("ip").as_string();
            std::vector<std::string> ip_address_elements;
            boost::split(ip_address_elements, ip_address, boost::is_any_of("/"));
            ip_agents[vnf_name] = ip_address_elements[0];

            log("Agent VNF %s created", vnf_name.c_str());

            return;
        } else {
            TTCN_error("Could not start Agent VNF. Status: %d", response.status_code());
        }
    } catch (const http_exception &e) {
        TTCN_error("Could not start Agent VNF: %s", e.what());
    }
}

/**
 * Stops all Agent VNFs 
 */
void ManoMsg::stopAllAgents() {
    log("Stopping all Agents");

    for(std::string agent: running_agents) {
        stopAgent(agent);
    }

    running_agents.clear();
}

/**
 * Stops an Agent
 * @param vnf_name The VNF that should be stopped
 */
void ManoMsg::stopAgent(std::string vnf_name) {
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

            return;
        } else {
            TTCN_error("Could not stop Agent. Status: %d", response.status_code());
        }
    } catch (const http_exception &e) {
        TTCN_error("Could not stop Agent: %s", e.what());
    }
}

/**
 * Connects an Agent to an SFC
 * @param vnf_name The name of the VNF that should be connected to the SFC
 * @param vnf_cp The connection point that the VNF should be connected to
 */
void ManoMsg::connectAgentToSfc(std::string vnf_name, std::string vnf_cp) {
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

            return;
        } else {
            TTCN_error("Could not create Agent VNF. Status: %d", response.status_code());
        }
    } catch (const http_exception &e) {
        TTCN_error("Could not create Agent VNF: %s", e.what());
    }

    log("%s is connected to connection point %s", vnf_name.c_str(), vnf_cp.c_str());
}

/**
 * Start the local vim-emu Docker container
 */
void ManoMsg::startDockerContainer() {
    if(!manage_docker) {
        return;
    }

    log("Starting docker container");

    // TODO: test if container is running
    std::string cmd = "docker run --name vim-emu -d --rm --privileged --pid='host' -v /var/run/docker.sock:/var/run/docker.sock vim-emu-img > /dev/null";
    int status = std::system(cmd.c_str());

    if(status < 0) {
        TTCN_error("Could not start docker container!");
    }

    log("Started docker container");
}

/**
 * Stops all local Docker containers
 */
void ManoMsg::stopDockerContainer() {
    if(!manage_docker) {
        return;
    }

    log("Stopping docker container");
    std::string cmd = "docker stop $(docker ps -q) > /dev/null";
    int status = std::system(cmd.c_str());

    if(status < 0) {
        TTCN_error("Could not stop docker container!");
    }

    log("Stopped docker container");
}

/**
 * Start a program on the local machine
 * @param command Command that should be started
 * @return The stdout of the program
 */
std::string ManoMsg::start_local_program(std::string command) {
    boost::asio::io_service ios;

    std::future<std::string> data;
    std::error_code ec;

    boost::process::child c(command,
            boost::process::std_in.close(),
            boost::process::std_out > data,
            boost::process::std_err > boost::process::null,
            ios);


    try {
        ios.run();
    } catch(const boost::process::process_error &e) {
        TTCN_error("There was an error while executing %s, %s", command.c_str(), e.what());
    }


    log("test2");
	try {
		auto output = data.get();
        if(output.size() > 0) {
            output.pop_back(); // Delete last character, e.g. last newline
        }

        return output;
    } catch (const boost::process::process_error &e) {
        TTCN_error("There was an error while executing %s, %s", command.c_str(), e.what());
    }
    log("test3");
    return "";
}


} /* end of namespace */
