/*
 * Copyright 2018 Christian Dröge <mail@cdroege.de>
 *
 * All rights reserved. This program and the accompanying materials are
 * made available under the terms of the Eclipse Public License v2.0 which
 * accompanies this distribution and is available at
 *
 * http://www.eclipse.org/legal/epl-v20.html
 */

#ifndef Reporter_HH
#define Reporter_HH

#include "TSP_PortType.hh"
#include <string>
#include <boost/filesystem.hpp>

namespace TSP__PortType {

class Reporter : public Reporter_BASE {
    public:
        Reporter(const char *par_port_name = NULL);
        ~Reporter();

        void set_parameter(const char *parameter_name,
                const char *parameter_value);

    private:
        bool debug;
        bool header;
        boost::filesystem::path subdirectory;
        boost::filesystem::path output_dir;
        boost::filesystem::path full_path;
        /* void Handle_Fd_Event(int fd, boolean is_readable,
           boolean is_writable, boolean is_error); */
        void Handle_Fd_Event_Error(int fd);
        void Handle_Fd_Event_Writable(int fd);
        void Handle_Fd_Event_Readable(int fd);
        /* void Handle_Timeout(double time_since_last_call); */
        void log(const char *fmt, ...);

        void save_metric(const TSP__Types::Save__Metric& send_par);
        void save_monitor_metrics(const TSP__Types::Save__Metric& send_par);

        template <typename tsp_types_list>
            void monitor_list_to_csv(tsp_types_list list, std::string metric_name, std::string vnf_name, int run, int interval);

    protected:
        void user_map(const char *system_port);
        void user_unmap(const char *system_port);

        void user_start();
        void user_stop();

        void outgoing_send(const TSP__Types::Save__Metric& send_par);
        void outgoing_send(const TSP__Types::Save__Status__Report& send_par);
};

} /* end of namespace */

#endif
