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
	/* void Handle_Fd_Event(int fd, boolean is_readable,
		boolean is_writable, boolean is_error); */
	void Handle_Fd_Event_Error(int fd);
	void Handle_Fd_Event_Writable(int fd);
	void Handle_Fd_Event_Readable(int fd);
	/* void Handle_Timeout(double time_since_last_call); */
    void log(const char *fmt, ...);

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
