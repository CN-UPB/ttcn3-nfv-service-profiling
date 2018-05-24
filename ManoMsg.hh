// This Test Port skeleton header file was generated by the
// TTCN-3 Compiler of the TTCN-3 Test Executor version CRL 113 200/6 R4A
// for dark (dark@development) on Wed May 23 15:46:39 2018

// Copyright (c) 2000-2018 Ericsson Telecom AB

// You may modify this file. Add your attributes and prototypes of your
// member functions here.

#ifndef ManoMsg_HH
#define ManoMsg_HH

#include "ServiceProfiling_PortType.hh"
#include <string>
#include <curl/curl.h>

namespace ServiceProfiling__PortType {

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

	/* Class members */
	bool debug;

	/* vim-emu state */
	std::string service_uuid;
	std::string service_instance_uuid;

	/* Config parameters */
	std::string vnf_path;
	std::string nsd_path;
	std::string rest_url;
	std::string rest_username;
	std::string rest_password;
	int port;

	/* curl */
	CURL *curl;
	struct curl_slist *chunk;
	std::string replyBuffer;
	static size_t replyToMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
	void setURL(std::string url);
	void performRestRequest();

	/* Other functions */
	void log(const char *fmt, ...) __attribute__((format(printf, 2, 3)));
protected:
	void user_map(const char *system_port);
	void user_unmap(const char *system_port);

	void user_start();
	void user_stop();

	void outgoing_send(const ServiceProfiling__Types::Setup__SFC& send_par);
	void outgoing_send(const ServiceProfiling__Types::Add__VNF& send_par);
	void outgoing_send(const ServiceProfiling__Types::Start__CMD& send_par);
};

} /* end of namespace */

#endif
