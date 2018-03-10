// avail_queue.h

// Includes


#include <vector>
#include <string>
#include <stdlib.h>
#include "sns.pb.h"
#include "sns.grpc.pb.h"





struct ip_ms_p {
	std::string ip;
	std::string m_port;
	std::string s_port;
	//std::unique_ptr<csce438::SNSService::Stub> m_stub;
	//std::unique_ptr<csce438::SNSService::Stub> s_stub;
	/*explicit*/ip_ms_p() 
	{
		ip = "uninit'd";
		m_port = "uninit'd";
		s_port = "uninit'd";
		//m_stub = nullptr;
		//s_stub = nullptr;

	}
	ip_ms_p(std::string i, std::string m, std::string s)
	{
		ip = i;
		m_port = m;
		s_port = s;
		//m_stub = csce438::SNSService::NewStub(grpc::CreateChannel(ip + ":" + m_port, grpc::InsecureChannelCredentials()));
		//s_stub = csce438::SNSService::NewStub(grpc::CreateChannel(ip + ":" + s_port, grpc::InsecureChannelCredentials()));

	}
	
	/*
	ip_ms_p& operator=(const ip_ms_p b) {
		ip = b.ip;
		m_port = b.m_port;
		s_port = b.s_port;

		// This may not work out so well
		m_stub = std::move(b.m_stub);
		s_stub = std::move(b.s_stub);
		// this corrupts b
	}
	
	ip_ms_p(const ip_ms_p& b) {

		ip = b.ip;
		m_port = b.m_port;
		s_port = b.s_port;

		// This may not work out so well
		m_stub = std::move(b.m_stub);
		s_stub = std::move(b.s_stub);
	}
	*/
	//~ip_ms_p() = default;
};

class avail_queue {
	std::vector<ip_ms_p> server_pairs;
	int offset;
	//int semaphore;
public:
	// Default Constructor
	avail_queue();

	//Queue functions
	void enqueue(ip_ms_p server);
	void next();
	ip_ms_p* get_avail_server();
	int get_size();
	ip_ms_p* get_elem(std::string e_ip, std::string e_m_port);
	ip_ms_p* elem_at(int i);
};