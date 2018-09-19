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
            /* void Handle_Fd_Event(int fd, boolean is_readable,
               boolean is_writable, boolean is_error); */
            void Handle_Fd_Event_Error(int fd);
            void Handle_Fd_Event_Writable(int fd);
            void Handle_Fd_Event_Readable(int fd);
            /* void Handle_Timeout(double time_since_last_call); */

            void send_successful_operation_status();

        protected:
            void user_map(const char *system_port);
            void user_unmap(const char *system_port);

            void user_start();
            void user_stop();

            void outgoing_send(const TSP__Types::Environment__Request& send_par);
            void outgoing_send(const TSP__Types::Setup__SFC& send_par);
            void outgoing_send(const TSP__Types::Add__Agents& send_par);
            void outgoing_send(const TSP__Types::Add__Monitors& send_par);
            void outgoing_send(const TSP__Types::Start__CMD& send_par);
            void outgoing_send(const TSP__Types::Set__Parameter__Config& send_par);
            void outgoing_send(const TSP__Types::Cleanup__Request& send_par);
    };

} /* end of namespace */

#endif
