#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <fstream>

using namespace std;

const int BUFFER_SIZE = 1024;

enum Direction{
	UP,
	DOWN,
	UNSET
};

struct Params{
	string host;
	string  port;
	Direction d;
	string what;
};

int main(int argc, char **argv){

	Params params{"", "", UNSET, ""};
	int c;
	opterr = 0;
	while ((c = getopt (argc, argv, ":h:p:d:u")) != -1)
		switch (c){
			case 'h':
				params.host = optarg;
				break;
			case 'p':
				params.port = optarg;
				break;
			case 'd':
				if (params.d == UNSET){
        			params.d = DOWN;
					params.what = optarg;
				}
				else{
					cerr << "Could be only one parameter -d|-u" << endl;
					return EXIT_FAILURE;
				}
				break;
			case 'u':
				if (params.d == UNSET){
					params.d = UP;
					params.what = optarg;
				}
				else{
					cerr << "Could be only one parameter -d|-u" << endl;
					return EXIT_FAILURE;
				}
 			default:
				cerr  << "Unrecognized parameter: " << optarg << endl;
				return EXIT_FAILURE;
      	}
	
	if (params.host == ""){
		cerr << "Host parameter is missing" << endl;
		return EXIT_FAILURE;
	}
	if (params.port == ""){
		cerr << "Port parameter is missing" << endl;
		return EXIT_FAILURE;
	}
	if (params.d == UNSET){
		cerr << "[-d|-u] is missing" << endl;
		return EXIT_FAILURE;
	}	

	int status;
	struct addrinfo host_info;
  	struct addrinfo *host_info_list, *i;

  	memset(&host_info, 0, sizeof host_info);
 
	host_info.ai_family = AF_UNSPEC;
	host_info.ai_socktype = SOCK_STREAM;

	status = getaddrinfo(params.host.c_str(), params.port.c_str(), &host_info, &host_info_list);
	if (status != 0){
		cerr << "getaddrinfo error: " << gai_strerror(status) << endl;
		return EXIT_FAILURE;
	}

	int socketfd;
	for (i = host_info_list; i != NULL; i = i->ai_next){
		socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
		if (socketfd == -1)
			continue;

		status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
		if (status != -1)
			break;

		close(socketfd);
	}

	if (i == NULL){
		cerr << "couldn't connect\n";
		freeaddrinfo(host_info_list);
		return EXIT_FAILURE;
	}

	if (params.d == UP){
		string msg = "Hello World";
		send(socketfd, msg.c_str(), msg.length(), 0);
	}
	else {
		int bytes;
		long bytes_received = 0;
		string data = "";
		char incoming_data_buffer[BUFFER_SIZE];		 
		memset(incoming_data_buffer, 0, BUFFER_SIZE);
	
		while ((bytes = recv(socketfd, incoming_data_buffer, BUFFER_SIZE, 0))){
			bytes_received += bytes;
			if (bytes == -1){
				cerr << "recieve error!" << endl ;
				close(socketfd);
				freeaddrinfo(host_info_list);
				return EXIT_FAILURE;
			}
			else data.append(incoming_data_buffer, bytes);
			memset(incoming_data_buffer, 0, BUFFER_SIZE);
		}
	}
	freeaddrinfo(host_info_list);
	return EXIT_SUCCESS;
}
