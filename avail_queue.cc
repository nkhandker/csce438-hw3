// avail_queue.cc
#include "avail_queue.h"
// TODO: implement semaphore for access safety
avail_queue::avail_queue(){
	server_pairs=*(new std::vector<ip_ms_p>());
	offset = 0;

}

void avail_queue::enqueue(ip_ms_p server) {
	// check pairs for duplicates
	//for (ip_ms_p m : this->server_pairs) {
	for (int i = 0; i < server_pairs.size(); ++i) {
		// check if already exists
		if (server.ip == server_pairs[i].ip) {
			if (server.m_port == server_pairs[i].m_port) {
				// already exists
				return;
			}
			else if (server.m_port == server_pairs[i].s_port) {
				// special case slave promoted adjust
				server_pairs[i].m_port = server.m_port;
				server_pairs[i].s_port = server.s_port;
				//server_pairs[i].m_stub = std::move(server.m_stub);
				//server_pairs[i].s_stub = std::move(server.s_stub);
				return;
			}
			else {
				// error
				return;
			}
		}
	}
	server_pairs.push_back(server);
}

void avail_queue::next() {
	offset = (offset + 1) % (server_pairs.size());
}

ip_ms_p* avail_queue::get_avail_server() {
	return &server_pairs[offset];
}

int avail_queue::get_size() {
	return server_pairs.size();
}

ip_ms_p* avail_queue::get_elem(std::string e_ip, std::string e_m_port) {
	for (int i = 0; i < server_pairs.size(); ++i) {
		if (e_ip == server_pairs[i].ip) {
			if (e_m_port == server_pairs[i].m_port) {
				return &server_pairs[i];
			}
		}
	}
	return nullptr;
}

ip_ms_p* avail_queue::elem_at(int i) {
	return &server_pairs.at(i);
}