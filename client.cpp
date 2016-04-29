/*
 * author:
 * nick: xsroka00
 * name: Radovan Sroka
*/


#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
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
	unsigned int port;
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
	
	Params params{"", 0, UNSET, ""};
	int c;
	char * ptr = nullptr;
	char * PORT = nullptr;

	opterr = 0;
	while ((c = getopt (argc, argv, "h:p:d:u:")) != -1)
		switch (c){
			case 'h':
				params.host = optarg;
				break;
			case 'p':
				PORT = optarg;
				params.port = strtoul(PORT, &ptr, 0 );
				if (*ptr != '\0'){
					cerr  << "Unrecognized port: " << PORT << endl;
					return EXIT_FAILURE;
				}

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
	if (params.port == 0){
		cerr << "Port parameter is missing" << endl;
		return EXIT_FAILURE;
	}
	if (params.d == UNSET){
		cerr << "[-d|-u] is missing" << endl;
		return EXIT_FAILURE;
	}	

	struct sockaddr_in serv_addr;
	struct hostent *server;
  	
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0){
		perror("ERROR opening socket");
		return EXIT_FAILURE;
	}

	server = gethostbyname(params.host.c_str());
	if (server == NULL){
		cerr << "ERROR, no such host" << endl;
		return EXIT_FAILURE;
	}

	memset(&serv_addr, 0, sizeof(sockaddr_in));
	serv_addr.sin_family = AF_INET;
	memcpy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(params.port);

	if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		cerr << "Cannot connect" << endl;
		return EXIT_FAILURE;
	}

	if (params.d == UP){
		long file_size = GetFileSize(params.what);
		if (file_size == -1){
			close(sockfd);
			cerr << "File doesn't exist" << endl;
			return EXIT_FAILURE;
		}

		char * data = new char[file_size];
		ifstream f;
		f.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
		try{
			f.open(params.what.c_str(), ios::in | ios::binary);
			f.read(data, file_size);
			f.close();
		} catch (exception const& e){
			cerr << "Couldn't read from file: " << params.what << endl;
			close(sockfd);
			return EXIT_FAILURE;
		}

		string msg = "UPLOAD:" + params.what + ";" + "FILE_SIZE:" + IntToString(file_size) + ";\n";
		int msg_size = msg.length();
		msg.append(data, file_size);
		send(sockfd, msg.c_str(), msg_size + file_size, 0);
		shutdown(sockfd, SHUT_WR);
		delete [] data;

		int bytes;
		long bytes_received = 0;
		string rcv = "";
		char incoming_data_buffer[BUFFER_SIZE];
		memset(incoming_data_buffer, 0, BUFFER_SIZE);

		while ((bytes = recv(sockfd, incoming_data_buffer, BUFFER_SIZE, 0))){
			bytes_received += bytes;
			if (bytes == -1){
				cerr << "recieve error!" << endl ;
				close(sockfd);
				return EXIT_FAILURE;
			}
			else rcv.append(incoming_data_buffer, bytes);
			memset(incoming_data_buffer, 0, BUFFER_SIZE);
		}

		if (rcv.find("ERROR") != string::npos){
			cerr << "file: " << params.what.c_str() << " cannot upload" << endl ;
			close(sockfd);
			return EXIT_FAILURE;
		}
	}
	else {
		string msg = "DOWNLOAD:" + params.what + ";\n";
		send(sockfd, msg.c_str(), msg.length(), 0);
		shutdown(sockfd, SHUT_WR);
		
		int bytes;
		long bytes_received = 0;
		string data = "";
		char incoming_data_buffer[BUFFER_SIZE];
		memset(incoming_data_buffer, 0, BUFFER_SIZE);
		
		while ((bytes = recv(sockfd, incoming_data_buffer, BUFFER_SIZE, 0))){
			bytes_received += bytes;
			if (bytes == -1){
				cerr << "recieve error!" << endl ;
				close(sockfd);
				return EXIT_FAILURE;
			}
			else data.append(incoming_data_buffer, bytes);
			memset(incoming_data_buffer, 0, BUFFER_SIZE);
		}

		string header = data.substr(0, data.find(";\n"));
		string body = data.substr(data.find(";\n") + 2);

		if (header.find("ERROR") != string::npos){
			cerr << "file: " << params.what.c_str() << " doesn't exist" << endl ;
			close(sockfd);
			return EXIT_FAILURE;
		}
		else if (header.find("DOWNLOAD:") != string::npos){
			int offset = 9;
			string file_name = header.substr(offset, header.find_first_of(";", offset) - offset);
			ofstream f;
			f.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
			try{
				string tmp_file = file_name + ".tmp";
				f.open(tmp_file, ios::out | ios::binary);
				f << body;
				f.close();
				if (rename(tmp_file.c_str(), file_name.c_str())){
					cerr << "rename failed" << endl;
					close(sockfd);
				}
			} catch (exception const& e){
				cerr << "Couldn't write to file " << file_name << endl;
				close(sockfd);
				return EXIT_FAILURE;
			}
		}
	}
	close(sockfd);
	return EXIT_SUCCESS;
}
