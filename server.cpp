#include <iostream>
#include <string>

#include <cstring> 
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/stat.h>

#include <fstream>
#include <sstream>

#define BACKLOG	 50

using namespace std;

const int BUFFER_SIZE = 2048;

string IntToString (long a){
	ostringstream temp;
	temp<<a;
	return temp.str();
}

long GetFileSize(string filename){
	struct stat stat_buf;
	int rc = stat(filename.c_str(), &stat_buf);
	return rc == 0 ? stat_buf.st_size : -1;
}

void * handle(void *pnewsock){

	cout << "I'm in thread" << endl;
	
	int bytes;
	long bytes_received = 0;
	string data = "";
	char incoming_data_buffer[BUFFER_SIZE];
	memset(incoming_data_buffer, 0, BUFFER_SIZE);
	cout << "bfr while recv" << endl;
	while ((bytes = recv(*(int *)pnewsock, incoming_data_buffer, BUFFER_SIZE, 0))){
		bytes_received += bytes;
		if (bytes == -1){
			cerr << "recieve error!" << endl ;
			close(*(int*)pnewsock);
			return NULL;
		}
		else data.append(incoming_data_buffer, bytes);
		memset(incoming_data_buffer, 0, BUFFER_SIZE);
	}

	if (data == ""){
		close(*(int *)pnewsock);
		cout << "End of thread" << endl;
		return NULL;
	}

	string header = data.substr(0, data.find(";\n"));
	string body = data.substr(data.find(";\n") + 2);
	
	cout << "bfr if" << endl;
	cout << data << endl;
	
	int offset = 0;
	if (header.find("UPLOAD:") != string::npos){
		offset = 7;
		string file_name = header.substr(offset, header.find_first_of(";", offset) - offset);
		cout << file_name << endl;
		ofstream f;
		f.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
		string msg = "UPLOAD:OK;\n";
		try{
			f.open(file_name+"1", ios::out | ios::binary);
			f << body;
			f.close();
		} catch (exception const& e){
			cerr << "Couldn't write to file " << file_name << endl;
			msg = "ERROR;\n";
		}
		send(*(int *)pnewsock, msg.c_str(), msg.length(), 0);
	}


	else if (header.find("DOWNLOAD:") != string::npos){
		cout << "DOWNLOAD" << endl;
		offset = 9;
		string file_name = header.substr(offset, header.find_first_of(";", offset) - offset);
		cout << file_name << endl;
		
		long file_size = GetFileSize(file_name);
		if (file_size == -1){
			cerr << "File doesn't exist" << endl;
			file_size = 1;
		}
		
		char * fl = new char[file_size];
		ifstream f;
		f.exceptions (ifstream::failbit | ifstream::badbit);
		string msg = "DOWNLOAD:" + file_name + ";" + "FILE_SIZE:" + IntToString(file_size) + 
				 ";" + "PART_NUM:" + "aaa" + ";" + "PART_SIZE:" + "aaa" + ";\n";
		
		cout << "bfr try" << endl;
		try{
			f.open(file_name, ios::in | ios::binary);
			f.read(fl, file_size);
			f.close();
		} catch (exception const& e){
			msg = "ERROR;\n";
			cout << msg;
			*fl = '\0';
			file_size = 0;
		}		
		
		cout << "after try" << endl;

		int msg_size = msg.length();
		msg.append(fl, file_size);
		send(*(int *)pnewsock, msg.c_str(), msg_size + file_size, 0);
		delete [] fl;
	}
	//sleep(2000);
	close(*(int *)pnewsock);
	cout << "End of thread" << endl;
	return NULL;
}

int main(int argc, char **argv){
	int sock;
	pthread_t thread;
	struct addrinfo hints, *res = NULL;
	char * PORT = NULL;
	int reuseaddr = 1;
	int status = 0;

	if (argc != 3){
		cerr << "There is only one parameter -p as port" << endl;
		return EXIT_FAILURE;
	}

	int c;
	opterr = 0;
	while ((c = getopt (argc, argv, "p:")) != -1)
		switch (c){
			case 'p':
				PORT = optarg;
				break;
 			default:
				cerr  << "Unrecognized parameter: " << optarg << endl;
				return EXIT_FAILURE;
				break;
	  	}

	cout << PORT << endl;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	
	status = getaddrinfo(NULL, PORT, &hints, &res);
	if (status != 0){
		cerr << "getaddrinfo error: " << gai_strerror(status) << endl;
		return EXIT_FAILURE;
	}

	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sock == -1) {
		cerr << "Couldn't create socket" << endl;
		return EXIT_FAILURE;
	}

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int)) == -1) {
		cerr << "setsockopt: failed" << endl;
		return EXIT_FAILURE;
	}

	if (bind(sock, res->ai_addr, res->ai_addrlen) == -1) {
		cerr << "Couldn't bind" << endl;
		return EXIT_FAILURE;
	}

	freeaddrinfo(res);

	if (listen(sock, BACKLOG) == -1) {
		cerr << "Listen: failed" << endl;
		return EXIT_FAILURE;
	}

	while (1) {
		printf("In while\n");
		socklen_t size = sizeof(struct sockaddr_in);
		struct sockaddr_in their_addr;
		int newsock = accept(sock, (struct sockaddr*)&their_addr, &size);
		if (newsock == -1) {
			cerr << "Accept: failed" << endl;
		}
		else {
			printf("Got a connection from %s on port %d\n", 
					inet_ntoa(their_addr.sin_addr), htons(their_addr.sin_port));
			if (pthread_create(&thread, NULL, handle, &newsock) != 0) {
				fprintf(stderr, "Failed to create thread\n");
			}
		}
	}

	close(sock);

	return EXIT_SUCCESS;
}

