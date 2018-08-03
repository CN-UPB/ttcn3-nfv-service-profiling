#ifndef ManoMsg_HH
#define ManoMsg_HH

#include "TSP_PortType.hh"
#include <string>
#include <vector>

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

	/* Class members */
	bool debug;
	bool debug_http;
	bool manage_docker;

	/* vim-emu state */
	std::string sfc_service_uuid;
	std::string sfc_service_instance_uuid;
	std::vector<std::string> running_vnfs;

	/* Config parameters */
	std::string vnf_path;
	std::string nsd_path;
	std::string gatekeeper_rest_url;
	std::string vimemu_rest_url;
	std::string rest_username;
	std::string rest_password;
	int port;

	/* Other functions */
	void log(const char *fmt, ...) __attribute__((format(printf, 2, 3)));
	std::string uploadPackage(std::string filepath);
	std::string startSfcService(std::string service_uuid);
	void stopSfcService(std::string service_uuid, std::string service_instance_uuid);
	void connectVnfToSfc(std::string vnf_name, std::string vnf_cp);
	void startVNF(std::string vnf_name, std::string vnf_image);
	void stopAllVNF();
	void stopVNF(std::string vnf_name);
	void startDockerContainer();
	void stopDockerContainer();
    std::string start_local_program(std::string);
protected:
	void user_map(const char *system_port);
	void user_unmap(const char *system_port);

	void user_start();
	void user_stop();

	void outgoing_send(const TSP__Types::Setup__SFC& send_par);
	void outgoing_send(const TSP__Types::Add__VNF& send_par);
	void outgoing_send(const TSP__Types::Start__CMD& send_par);
	void outgoing_send(const TSP__Types::Set__Parameter__Config& send_par);
	void outgoing_send(const TSP__Types::Cleanup__Request& send_par);
};

} /* end of namespace */

#endif
