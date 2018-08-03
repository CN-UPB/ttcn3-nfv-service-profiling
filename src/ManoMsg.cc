#include "ManoMsg.hh"
#include "OutputParser.hh"
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>
#include <cstdlib>
#include <boost/algorithm/string.hpp>
#include <boost/process.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <thread>
#include <regex>


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
    nsd_path = "/home/dark/son-examples/service-projects/";

    gatekeeper_rest_url = "http://172.17.0.2:5000";
    vimemu_rest_url = "http://172.17.0.2:5001";
    rest_username = "admin";
    rest_password = "admin";

    manage_docker = true;

    //sfc_service_instance_uuid = "";
    //sfc_service_uuid = "";
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
 * TTCN-3 unmap operation. Cleans up everything, e.g. stop a measurment point VNFs, stops the
 * SFC and stop the vim-emu container
 */
void ManoMsg::user_unmap(const char * /*system_port*/)
{
    stopAllVNF();

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
 * Handles a send operation for an Add_VNF request, e.g. start a measurment point VNF
 * @param send_par Add_VNF request containing vnf_name, connection point and image
 */
void ManoMsg::outgoing_send(const TSP__Types::Add__VNF& send_par)
{
    std::string vnf_name = std::string(((const char*)send_par.name()));
    std::string vnf_cp = std::string(((const char*)send_par.connection__point()));
    std::string vnf_image = std::string(((const char*)send_par.image()));

    log("Setting up VNF %s with connection point %s from image %s", vnf_name.c_str(), vnf_cp.c_str(), vnf_image.c_str());

    startVNF(vnf_name, vnf_image);
    connectVnfToSfc(vnf_name, vnf_cp);
}

/**
 * Handles a Cleanup_Request. Stops all VNFs.
 */
void ManoMsg::outgoing_send(const TSP__Types::Cleanup__Request& /*send_par*/)
{
    log("Cleanup vim-emu!");

    stopAllVNF();
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
    std::string vnf_name = std::string(((const char*)send_par.vnf()));
    std::string cmd = std::string(((const char*)send_par.cmd()));

    OPTIONAL<CHARSTRING> output_parser_optional = ((const OPTIONAL<CHARSTRING>)send_par.output__parser());
    std::string output_parser;

    if(output_parser_optional.is_present()) {
        output_parser = output_parser_optional();
    }

    // replace macros in commands
    std::smatch match;
    std::regex IP4("\\$\\{IP4:(.*)\\}");
    if(std::regex_search(cmd, match, IP4)) {
        std::ssub_match sub_match = match[1];
        std::string ip4_name = sub_match.str();

        auto ip4_address = start_local_program("docker inspect -f \"{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}\" mn." + ip4_name);
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
        boost::process::spawn(command);
    } else {
        auto command_stdout = start_local_program(command);

        auto metric = OutputParser::parse(command_stdout, output_parser);

        TSP__Types::Start__CMD__reply cmd_reply;
        cmd_reply.metric() = CHARSTRING(metric.c_str());
        incoming_message(cmd_reply);
    }
}

/**
 * Handles the send operation for Set_Parameter_Config requests
 * @param send_par Set_Parameter_Config request (e.g. service name, function id and resource config
 */
void ManoMsg::outgoing_send(const TSP__Types::Set__Parameter__Config& send_par)
{
    std::string service_name = std::string(((const char*)send_par.service__name()));
    std::string vnf_name = std::string(((const char*)send_par.paramcfg().function__id()));

    // All the possible values that vim-emu supports
    int vcpus = (const int)send_par.paramcfg().vcpus();
    int memory = (const int)send_par.paramcfg().memory();
    int storage = (const int)send_par.paramcfg().storage();

    // Additional parameter configuration values
    //auto rvalues = (const TSP__Types::RessourceValues)send_par.resourcecfg().resource__values();
    //for(int i = 0; i < rvalues.size_of() ; i++) {
    //    if(rvalues[i].name() == "vcpu") {
    //        vcpus = rvalues[i].actual__value();
    //    } else if(rvalues[i].name() == "memory") {
    //        memory = rvalues[i].actual__value();
    //    }
    //}

    std::vector<std::string> service_name_elements;
    boost::split(service_name_elements, service_name, boost::is_any_of("."));

    log("Setting resource configuration of VNF %s", vnf_name.c_str());

    // TODO
    //std::string filename = nsd_path  + service_name_elements[0] + "-emu" + "/sources/vnf/" + vnf_name + "/" + vnf_name + "d.yml";
    std::string filename = "/home/dark/son-examples/service-projects/sonata-snort-service-emu/sources/vnf/snort-vnf/snort-vnfd.yml";

    log("Filename %s", filename.c_str());

    std::string cmd = "python3 /home/dark/nfv-service-profiling/bin/set-resource-configuration.py " + filename + " " + std::to_string(vcpus) + " " + std::to_string(memory) + " " + std::to_string(storage);
    log("Command: %s", cmd.c_str());

    std::system(cmd.c_str());

    //std::system("son-package --project sonata-snort-service-emu -n sonata-snort-service");

    log("Setting resource configuration of VNF %s completed", vnf_name.c_str());
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
 * Start a measurement point VNF
 * @param vnf_name Name of the VNF
 * @param vnf_image Image of the VNF
 */
void ManoMsg::startVNF(std::string vnf_name, std::string vnf_image) {
    log("Start measurement point VNF with name %s from image %s", vnf_name.c_str(), vnf_image.c_str());

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
            running_vnfs.push_back(vnf_name);
            log("Measurement point VNF %s created", vnf_name.c_str());

            return;
        } else {
            TTCN_error("Could not start Measurement point VNF. Status: %d", response.status_code());
        }
    } catch (const http_exception &e) {
        TTCN_error("Could not start Measurement point VNF: %s", e.what());
    }
}

/**
 * Stops all measurement point VNFs 
 */
void ManoMsg::stopAllVNF() {
    log("Stopping all measurement point VNFs");

    for(std::string vnf_name : running_vnfs) {
        stopVNF(vnf_name);
    }

    running_vnfs.clear();
}

/**
 * Stops a measurement point VNF
 * @param vnf_name The VNF that should be stopped
 */
void ManoMsg::stopVNF(std::string vnf_name) {
    log("Stopping measurement point VNF %s", vnf_name.c_str());

    http_client client(vimemu_rest_url);
    auto query = uri_builder("/restapi/compute/dc1/" + vnf_name).to_string(); // TODO: dc1 configureable

    http_request req(methods::DEL);
    req.set_request_uri(query);

    try {
        http_response response = client.request(req).get();

        if(response.status_code() == status_codes::OK) {
            //running_vnfs.push_back(vnf_name); TODO: Reverse operation (delete)
            log("Measurement point VNF %s stopped", vnf_name.c_str());

            return;
        } else {
            TTCN_error("Could not stop Measurement point VNF. Status: %d", response.status_code());
        }
    } catch (const http_exception &e) {
        TTCN_error("Could not stop Measurement point VNF: %s", e.what());
    }
}

/**
 * Connects a VNF to a SFC
 * @param vnf_name The name of the VNF that should be connected to the SFC
 * @param vnf_cp The connection point that the VNF should be connected to
 */
void ManoMsg::connectVnfToSfc(std::string vnf_name, std::string vnf_cp) {
    log("Connect %s to connection point %s", vnf_name.c_str(), vnf_cp.c_str());

    std::vector<std::string> vnf_cp_elements;
    boost::split(vnf_cp_elements, vnf_cp, boost::is_any_of(":"));

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
            TTCN_error("Could not create Measurement point VNF. Status: %d", response.status_code());
        }
    } catch (const http_exception &e) {
        TTCN_error("Could not create Measurement point VNF: %s", e.what());
    }
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

    boost::process::child c(command,
            boost::process::std_in.close(),
            boost::process::std_out > data, //so it can be written without anything
            boost::process::std_err > boost::process::null,
            ios);


    ios.run();

    auto output = data.get();
    output.pop_back(); // Delete last character, e.g. last newline

    return output;
}


} /* end of namespace */
