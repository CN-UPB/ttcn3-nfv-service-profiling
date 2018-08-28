#ifndef ManoMsg_HH
#define ManoMsg_HH

#include "TSP_PortType.hh"
#include <cpprest/json.h>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <future>

namespace TSP__PortType {

    class ManoMsg : public ManoMsg_BASE {
        public:
            ManoMsg(const char *par_port_name = NULL);
            ~ManoMsg();

            void set_parameter(const char *parameter_name,
                    const char *parameter_value);

        private:
            class Monitor {
                private:
                    std::string vnf_name;
                    std::vector<std::string> metrics;
                    std::string docker_rest_url;
                    boolean running;
                    int interval;

                    static double calculate_cpu_percent(web::json::value json);

                public:
                    Monitor(std::string vnf_name, std::vector<std::string> metrics, int interval, std::string docker_rest_url);
                    std::future<std::map<std::string, std::map<std::string, std::vector<std::string>>>> run();
                    void stop();
            };

            /* void Handle_Fd_Event(int fd, boolean is_readable,
               boolean is_writable, boolean is_error); */
            void Handle_Fd_Event_Error(int fd);
            void Handle_Fd_Event_Writable(int fd);
            void Handle_Fd_Event_Readable(int fd);
            /* void Handle_Timeout(double time_since_last_call); */

            /* Class members */
            bool debug;
            bool debug_http;
            bool manage_docker;
            bool metric_collecting;
            std::map<std::string, std::vector<std::string>> monitor_output;

            /* vim-emu state */
            std::string sfc_service_uuid;
            std::string sfc_service_instance_uuid;
            std::vector<std::string> running_agents;
            std::map<std::string, std::string> ip_agents;
            std::map<std::string, std::map<std::string, std::string>> vnf_additional_parameters;

            /* Adapter state */
            std::vector<std::future<std::map<std::string, std::map<std::string, std::vector<std::string>>>>> monitor_futures;
            std::vector<Monitor*> monitor_objects;

            /* Config parameters */
            std::string vnf_path;
            std::string nsd_path;
            std::string docker_rest_url;
            std::string gatekeeper_rest_url;
            std::string vimemu_rest_url;
            std::string rest_username;
            std::string rest_password;
            int port;

            /* Other functions */
            void log(const char *fmt, ...) __attribute__((format(printf, 2, 3)));
            std::string uploadPackage(std::string filepath);
            std::string startSfcService(std::string service_uuid);
            void apply_additional_parameters_for_sfc();
            void stopSfcService(std::string service_uuid, std::string service_instance_uuid);
            void connectAgentToSfc(std::string vnf_name, std::string vnf_cp);
            void startAgent(std::string vnf_name, std::string vnf_image);
            void stopAllAgents();
            void stopAgent(std::string vnf_name);
            void startDockerContainer();
            void stopDockerContainer();
            std::string start_local_program(std::string, bool background=false);


        protected:
            void user_map(const char *system_port);
            void user_unmap(const char *system_port);

            void user_start();
            void user_stop();

            void outgoing_send(const TSP__Types::Setup__SFC& send_par);
            void outgoing_send(const TSP__Types::Add__Agents& send_par);
            void outgoing_send(const TSP__Types::Add__Monitors& send_par);
            void outgoing_send(const TSP__Types::Start__CMD& send_par);
            void outgoing_send(const TSP__Types::Set__Parameter__Config& send_par);
            void outgoing_send(const TSP__Types::Cleanup__Request& send_par);
    };

} /* end of namespace */

#endif
