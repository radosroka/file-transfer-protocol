#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netdb.h>
#include <unistd.h>

#include <fstream>
#include <sstream>

using namespace std;

const int BUFFER_SIZE = 1024;

string IntToString (long a){
	ostringstream temp;
	temp<<a;
	return temp.str();
}

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

long GetFileSize(string filename)
{
	struct stat stat_buf;
	int rc = stat(filename.c_str(), &stat_buf);
	return rc == 0 ? stat_buf.st_size : -1;
}

int main(int argc, char **argv){
	
	cout << "WELCOME" << endl;
	Params params{"", "", UNSET, ""};
	int c;
	opterr = 0;
	while ((c = getopt (argc, argv, "h:p:d:u:")) != -1)
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
				break;
 			default:
				cerr  << "Unrecognized parameter: " << optarg << endl;
				return EXIT_FAILURE;
				break;
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

	cout << params.host << endl;
	cout << params.port << endl;
	
	cout << "bfr getaddrinfo" << endl;
	status = getaddrinfo(params.host.c_str(), params.port.c_str(), &host_info, &host_info_list);
	if (status != 0){
		cerr << "getaddrinfo error: " << gai_strerror(status) << endl;
		return EXIT_FAILURE;
	}

	int socketfd;
	for (i = host_info_list; i != NULL; i = i->ai_next){
		socketfd = socket(i->ai_family, i->ai_socktype, i->ai_protocol);
		if (socketfd == -1)
			continue;

		status = connect(socketfd, i->ai_addr, i->ai_addrlen);
		if (status != -1)
			break;

		close(socketfd);
	}
	cout << "before freeaddrinfo" << endl;
	freeaddrinfo(host_info_list);
	if (i == NULL){
		cerr << "couldn't connect\n";
		return EXIT_FAILURE;
	}
	cout << "before if" << endl;
	if (params.d == UP){
		cout << "Start UPLOADING" << endl;
		long file_size = GetFileSize(params.what);
		if (file_size == -1){
			close(socketfd);
			cerr << "File doesn't exist" << endl;
			return EXIT_FAILURE;
		}
		cout << "FILE_SIZE is: " << file_size << endl;

		char * data = new char[file_size];
		ifstream f;
		f.open(params.what.c_str(), ios::in | ios::binary);
		f.read(data, file_size);
		f.close();
		string msg = "UPLOAD:" + params.what + ";" + "FILE_SIZE:" + IntToString(file_size) + ";\n";
		int msg_size = msg.length();
		msg.append(data, file_size);
		send(socketfd, msg.c_str(), msg_size + file_size, 0);
		delete [] data;

		int bytes;
		long bytes_received = 0;
		string rcv = "";
		char incoming_data_buffer[BUFFER_SIZE];
		memset(incoming_data_buffer, 0, BUFFER_SIZE);

		while ((bytes = recv(socketfd, incoming_data_buffer, BUFFER_SIZE, 0))){
			bytes_received += bytes;
			if (bytes == -1){
				cerr << "recieve error!" << endl ;
				close(socketfd);
				return EXIT_FAILURE;
			}
			else rcv.append(incoming_data_buffer, bytes);
			memset(incoming_data_buffer, 0, BUFFER_SIZE);
		}

		if (rcv.find("ERROR") != string::npos){
			cerr << "file: " << params.what.c_str() << " cannot upload" << endl ;
			close(socketfd);
			return EXIT_FAILURE;
		}
	}
	else {
		string msg = "DOWNLOAD:" + params.what + ";\n";
		cout << msg << endl;
		send(socketfd, msg.c_str(), msg.length(), 0);
		shutdown(socketfd, SHUT_WR);
		cout << "after send" << endl;
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
				return EXIT_FAILURE;
			}
			else data.append(incoming_data_buffer, bytes);
			memset(incoming_data_buffer, 0, BUFFER_SIZE);
		}

		cout << "after recv" << endl;

		string header = data.substr(0, data.find(";\n"));
		string body = data.substr(data.find(";\n") + 2);

		if (header.find("ERROR") != string::npos){
			cerr << "file: " << params.what.c_str() << " doesn't exist" << endl ;
			close(socketfd);
			return EXIT_FAILURE;
		}
		else if (header.find("DOWNLOAD:") != string::npos){
			int offset = 9;
			string file_name = header.substr(offset, header.find_first_of(";", offset) - offset);
			cout << file_name << endl;
			ofstream f;
			f.open(file_name + "2", ios::out | ios::binary);
			f << body;
			f.close();
		}
	}
	return EXIT_SUCCESS;
}
