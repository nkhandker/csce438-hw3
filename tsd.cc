/*
 *
 * Copyright 2015, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */



#include <ctime>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <cstdio>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>

#include "sns.grpc.pb.h"


// STILL TODO: 
// - implement data persistence (i.e. write lists and timeline to file) [all HW2 stuff]
// - implement lists backup/retrieval to/from router 

#include "avail_queue.h"

using google::protobuf::Timestamp;
using google::protobuf::Duration;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using csce438::Message;
using csce438::ListReply;
using csce438::Request;
using csce438::Reply;
using csce438::Ping;
using csce438::Ip_info;
using csce438::MS_Ip_info;
using csce438::SNSService;

struct Client {
  std::string username;
  bool connected = true;
  int following_file_size = 0;
  std::vector<Client*> client_followers;
  std::vector<Client*> client_following;
  ServerReaderWriter<Message, Message>* stream = 0;
  bool operator==(const Client& c1) const{
    return (username == c1.username);
  }
  int saveLists() {
	  std::vector<Client*>::const_iterator it;
	  std::ofstream temp_o;
	  std::string filename = username+"_followers_list.txt";

	  temp_o.open(filename.c_str());
	  for (it = client_followers.begin(); it != client_followers.end(); it++) {
		  Client *temp_client = *it;
		  temp_o << temp_client->username << std::endl;
	  }
	  temp_o.close();

	  filename = username + "_following_list.txt";
	  temp_o.open(filename.c_str());
	  for (it = client_following.begin(); it != client_following.end(); it++) {
		  Client *temp_client = *it;
		  temp_o << temp_client->username << std::endl;
	  }
	  temp_o.close();
	  return 0;
  }

  int loadLists(/*std::vector<Client>& database*/);

};

std::string router_ip_port = "192.168.0.1:3040";

// Our class that the router uses to keep track of MS pair connection data
 avail_queue available;

 //Global variables that hold connection data for master
 std::unique_ptr<SNSService::Stub> stub_to_r;
 std::string my_ip_addr;
 std::string my_port;
 char my_type;
 std::string router_ip;
 std::string router_port;

//Vector that stores every client that has been created
std::vector<Client> client_db;

// Helper function found at http://www.cplusplus.com/forum/beginner/27291/
void pause(int dur){
	int temp = time(NULL) + dur;

	while (temp > time(NULL));
}



//Helper function used to find a Client object given its username
int find_user(std::string username){
  int index = 0;
  for(Client c : client_db){
    if(c.username == username)
      return index;
    index++;
  }
  return -1;
}
int Client::loadLists() {
	std::ifstream temp_i;
	std::string filename = username + "_followers_list.txt";
	std::string readline = "";
	
	std::vector<Client*> followers;
	std::vector<Client*> following;

	temp_i.open(filename.c_str());
	while (std::getline(temp_i, readline)) {
		followers.push_back(&client_db[find_user(readline)]);
	}
	temp_i.close();

	filename = username + "_following_list.txt";
	readline = "";
	temp_i.open(filename.c_str());
	while (std::getline(temp_i, readline)) {
		following.push_back(&client_db[find_user(readline)]);
	}
	temp_i.close();

	client_followers=followers;
	client_following=following;
}

class SNSServiceImpl final : public SNSService::Service {  

  /* Client <---> Master */
  Status List(ServerContext* context, const Request* request, ListReply* list_reply) override {
    Client user = client_db[find_user(request->username())];
    int index = 0;
    for(Client c : client_db){
      list_reply->add_all_users(c.username);
    }
    std::vector<Client*>::const_iterator it;
    for(it = user.client_followers.begin(); it!=user.client_followers.end(); it++){
      list_reply->add_followers((*it)->username);
    }
    return Status::OK;
  }

  // actually is now Router <---> Master
  Status Follow(ServerContext* context, const Request* request, Reply* reply) override {
	  std::string username1 = request->username();
	  std::string username2 = request->arguments(0);
	  int join_index = find_user(username2);
	  if (join_index < 0 || username1 == username2)
		  reply->set_msg("Follow Failed -- Invalid Username");
	  else {
		  Client *user1 = &client_db[find_user(username1)];
		  Client *user2 = &client_db[join_index];
		  if (std::find(user1->client_following.begin(), user1->client_following.end(), user2) != user1->client_following.end()) {
			  reply->set_msg("Follow Failed -- Already Following User");
			  return Status::OK;
		  }
		  user1->client_following.push_back(user2);
		  user1->saveLists();
		  user2->client_followers.push_back(user1);
		  user2->saveLists();
		  reply->set_msg("Follow Successful");
	  }
	  return Status::OK;
  }

  // actually is now Router <---> Master
  Status UnFollow(ServerContext* context, const Request* request, Reply* reply) override {
    std::string username1 = request->username();
    std::string username2 = request->arguments(0);
    int leave_index = find_user(username2);
    if(leave_index < 0 || username1 == username2)
      reply->set_msg("UnFollow Failed -- Invalid Username");
    else{
      Client *user1 = &client_db[find_user(username1)];
      Client *user2 = &client_db[leave_index];
      if(std::find(user1->client_following.begin(), user1->client_following.end(), user2) == user1->client_following.end()){
	reply->set_msg("UnFollow Failed -- Not Following User");
        return Status::OK;
      }
      user1->client_following.erase(find(user1->client_following.begin(), user1->client_following.end(), user2));
	  user1->saveLists();
      user2->client_followers.erase(find(user2->client_followers.begin(), user2->client_followers.end(), user1));
	  user2->saveLists();

	  reply->set_msg("UnFollow Successful");
    }
    return Status::OK;
  }
  
  Status Login(ServerContext* context, const Request* request, Reply* reply) override {
    Client c;
    std::string username = request->username();
    int user_index = find_user(username);
    if(user_index < 0){
      c.username = username;
      client_db.push_back(c);
      reply->set_msg("Login Successful!");
    }
    else{ 
      Client *user = &client_db[user_index];
      if(user->connected)
        reply->set_msg("Invalid Username");
      else{
		  //load follow/following vectors for the client
		  user->loadLists();
        std::string msg = "Welcome Back " + user->username;
	reply->set_msg(msg);
        user->connected = true;
      }
    }
    return Status::OK;
  }

  Status Timeline(ServerContext* context, 
		ServerReaderWriter<Message, Message>* stream) override {
    Message message;
    Client *c;
    while(stream->Read(&message)) {
      std::string username = message.username();
      int user_index = find_user(username);
      c = &client_db[user_index];
 
      //Write the current message to "username.txt"
      std::string filename = username+".txt";
      std::ofstream user_file(filename,std::ios::app|std::ios::out|std::ios::in);
      google::protobuf::Timestamp temptime = message.timestamp();
      std::string time = google::protobuf::util::TimeUtil::ToString(temptime);
      std::string fileinput = time+" :: "+message.username()+":"+message.msg()+"\n";
      //"Set Stream" is the default message from the client to initialize the stream
      if(message.msg() != "Set Stream")
        user_file << fileinput;
      //If message = "Set Stream", print the first 20 chats from the people you follow
      else{
        if(c->stream==0)
      	  c->stream = stream;
        std::string line;
        std::vector<std::string> newest_twenty;
        std::ifstream in(username+"following.txt");
        int count = 0;
        //Read the last up-to-20 lines (newest 20 messages) from userfollowing.txt
        while(getline(in, line)){
          if(c->following_file_size > 20){
	    if(count < c->following_file_size-20){
              count++;
	      continue;
            }
          }
          newest_twenty.push_back(line);
        }
        Message new_msg; 
 	//Send the newest messages to the client to be displayed
	for(int i = 0; i<newest_twenty.size(); i++){
	  new_msg.set_msg(newest_twenty[i]);
          stream->Write(new_msg);
        }    
        continue;
      }
      //Send the message to each follower's stream
      std::vector<Client*>::const_iterator it;
      for(it = c->client_followers.begin(); it!=c->client_followers.end(); it++){
        Client *temp_client = *it;
      	if(temp_client->stream!=0 && temp_client->connected)
	  temp_client->stream->Write(message);
        //For each of the current user's followers, put the message in their following.txt file
        std::string temp_username = temp_client->username;
        std::string temp_file = temp_username + "following.txt";
	std::ofstream following_file(temp_file,std::ios::app|std::ios::out|std::ios::in);
	following_file << fileinput;
        temp_client->following_file_size++;
	std::ofstream user_file(temp_username + ".txt",std::ios::app|std::ios::out|std::ios::in);
        user_file << fileinput;
      }
    }
    //If the client disconnected from Chat Mode, set connected to false
    c->connected = false;
    return Status::OK;
  }

  
  
  /* Master <---> Router */
  Status Register_Pair(ServerContext* context, const MS_Ip_info* ms_ip, Ping* pong) override { 
	  available.enqueue(ip_ms_p(ms_ip->ip(),ms_ip->m_port(), ms_ip->s_port()));
	  pong->set_code(0);
	  return Status::OK;
  }
  // TODO: add update_router_userlists() to functions

  /* Router <---> Master */
  Status Are_You_There/*?*/(ServerContext* context, const Ping* pin, Ping* pong) override { 
	  pong->set_code(0);
	  return Status::OK;
  }

  /* Router <---> Slave  */
  Status Promote_S_to_M(ServerContext* context, const Ping* pin, Ping* pong) override { 
	  pong->set_code(0);

	  my_type = 'M';
	  int s_port = atoi(my_port.c_str()) + 1;
	  if (!fork()) {
		  execlp("./tsd", my_ip_addr.c_str(), std::to_string(s_port).c_str(), "S", NULL);
	  }
	  // create stub to well known router
	  stub_to_r = SNSService::NewStub(grpc::CreateChannel(router_ip_port, grpc::InsecureChannelCredentials()));
	  ClientContext my_context;
	  MS_Ip_info my_info;
	  Ping pong1;
	  my_info.set_ip(my_ip_addr);
	  my_info.set_m_port(my_port);
	  my_info.set_s_port(std::to_string(s_port));
	  // TODO: add cases for timeout then loop until register complete
	  Status stat = stub_to_r->Register_Pair(&my_context, my_info, &pong1);
	  if (stat.ok()) {
		  return Status::OK;
	  }
	  else {
		  bool can_not_exit = true;
		  while (can_not_exit) {
			  pause(1); // wait 1 second
			  ClientContext my_context1;
			  Status stat1 = stub_to_r->Register_Pair(&my_context1, my_info, &pong1);
			  if (stat1.ok()) {
				  return Status::OK;
			  }

		  }
	  }
  }

  /* Client <---> Router */
  Status Request_Avail_Server(ServerContext* context, const Ping* pong, Ip_info* ip) override {
	  ip_ms_p* a = available.get_avail_server();
	  ip->set_ip(a->ip);
	  ip->set_port(a->m_port);
	  return Status::OK;
  }

  Status Report_Comm_Err(ServerContext* context, const Ip_info* ip, Ping* pong) override {

	  ClientContext context1;
	  Ping ping1;
	  ping1.set_code(0);
	  Ping pong1;

	  // Search the available list for the Ip_info
	  ip_ms_p* problem_server = available.get_elem(ip->ip(), ip->port());
	  if (problem_server == nullptr) {
		  pong->set_code(2); // "request_avail_server() then try again"
		  return Status::OK;
	  }
	  // once found, use stub to invoke are_you_there?()
	  std::unique_ptr<csce438::SNSService::Stub> found_master_stub = csce438::SNSService::NewStub(grpc::CreateChannel(problem_server->ip + ":" + problem_server->m_port, grpc::InsecureChannelCredentials()));
	  Status stat1 = found_master_stub->Are_You_There/*?*/(&context1, ping1, &pong1);
	  //	if invoke works then report "try_again" status in pong
	  if (stat1.ok()) {
		  pong->set_code(1); // "try_again"
		  return Status::OK;
	  }
	  // else: invoke fails and new context created for slave stub to invoke promote
	  ClientContext context2;
	  Ping ping2;
	  ping2.set_code(0);
	  Ping pong2;
	  std::unique_ptr<csce438::SNSService::Stub> slave_stub = csce438::SNSService::NewStub(grpc::CreateChannel(problem_server->ip + ":" + problem_server->s_port, grpc::InsecureChannelCredentials()));
	  Status stat2 = slave_stub->Promote_S_to_M(&context2, ping2, &pong2);
	  // if works then report "request_avail_server then try again" status in pong
	  if (stat2.ok()) {
		  pong->set_code(2); // "request_avail_server() then try again"
		  return Status::OK;
	  }
	  // else: invoke fails so call available.next() and "request_avail_server then try again" status in pong
	  else {
		  available.next();
		  pong->set_code(2); // "request_avail_server() then try again"
		  return Status::OK;
	  }

	  pong->set_code(0); // Don't know what happend
	  return Status::OK;
  }

  Status Follow_plus(ServerContext* context, const Request* request, Reply* reply) override {
	  for (int i = 0; i < available.get_size(); ++i) {
		  ip_ms_p* m_server = available.elem_at(i);
		  std::unique_ptr<csce438::SNSService::Stub> master_stub = csce438::SNSService::NewStub(grpc::CreateChannel(m_server->ip + ":" + m_server->m_port, grpc::InsecureChannelCredentials()));
		  ClientContext context1;
		  Reply reply1;
		  Status stat = master_stub->Follow(&context1, *request, &reply1);
	  }
	  reply->set_msg("Follows pushed");
	  
	  return Status::OK;
  }

  Status UnFollow_plus(ServerContext* context, const Request* request, Reply* reply) override {
	  for (int i = 0; i < available.get_size(); ++i) {
		  ip_ms_p* m_server = available.elem_at(i);
		  std::unique_ptr<csce438::SNSService::Stub> master_stub = csce438::SNSService::NewStub(grpc::CreateChannel(m_server->ip + ":" + m_server->m_port, grpc::InsecureChannelCredentials()));
		  ClientContext context1;
		  Reply reply1;
		  Status stat = master_stub->UnFollow(&context1, *request, &reply1);
	  }
	  reply->set_msg("Unfollows pushed");

	  return Status::OK;
  }
};


void RunServer(std::string ip_addr,std::string port_no, char type) {
  std::string server_address = "0.0.0.0:"+port_no;
  SNSServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  //std::cout << "Server listening on " << server_address << std::endl;

  switch (type) {
	case 's':
	case 'S':
		/* Do slave stuff */break;
	case 'm':
	case 'M':
	{
		/* Do master stuff */
		// exec slave creation
		int s_port = atoi(my_port.c_str()) + 1;
		if (!fork()) {
			execlp("./tsd", my_ip_addr.c_str(), std::to_string(s_port).c_str(), "S", NULL);
		}
		// create stub to well known router
		stub_to_r = SNSService::NewStub(grpc::CreateChannel(router_ip_port, grpc::InsecureChannelCredentials()));
		// invoke Register_pair()
		ClientContext context;
		MS_Ip_info my_info;
		Ping pong;
		my_info.set_ip(my_ip_addr);
		my_info.set_m_port(my_port);
		my_info.set_s_port(std::to_string(s_port));

		// TODO: add cases for unavailable Router ie loop until router is avail
		Status stat = stub_to_r->Register_Pair(&context, my_info, &pong);
		if(stat.ok()){
                 std::cout << "fucking shit is connected" << std::endl; 
                }
                // Check status then break
		break;
	}
	case 'r':
	case 'R':
		/* Do router stuff */
		std::cout << "Server listening on " << server_address << std::endl;
		break;
	default:
		break;
  }
  
  server->Wait();
}

int main(int argc, char** argv) {

  // ip_addr may need to be a global variable
  my_ip_addr = "127.0.1.1";
  my_port = "3010";
  
  my_type = 'S';
  
  int opt = 0;
  while ((opt = getopt(argc, argv, "i:p:t:r:x:")) != -1){
    switch(opt) {
	  case 'i':
		  my_ip_addr = optarg; break;
          case 'p':
                  my_port = optarg; break;
	  case 't':
		  my_type = optarg[0]; break;
          case 'r': 
		  router_ip = optarg; break;
          case 'x': 
		  router_port = optarg; break;
      default:
	  std::cerr << "Invalid Command Line Argument\n";
    }
  }
  router_ip_port = router_ip + ":" + router_port;
  RunServer(my_ip_addr, my_port, my_type);

  return 0;
}
