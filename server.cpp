/*
 * author: 
 * nick: xsroka00
 * name: Radovan Sroka
*/

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

	int bytes;
	long bytes_received = 0;
	string data = "";
	char incoming_data_buffer[BUFFER_SIZE];
	memset(incoming_data_buffer, 0, BUFFER_SIZE);
	
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
		return NULL;
	}

	string header = data.substr(0, data.find(";\n"));
	string body = data.substr(data.find(";\n") + 2);
	
	int offset = 0;
	if (header.find("UPLOAD:") != string::npos){
		offset = 7;
		string file_name = header.substr(offset, header.find_first_of(";", offset) - offset);
		ofstream f;
		f.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
		string msg = "UPLOAD:OK;\n";
		try{
			string tmp_file = file_name + ".tmp";
			f.open(tmp_file, ios::out | ios::binary);
			f << body;
			f.close();
			if (rename(tmp_file.c_str(), file_name.c_str())){
				cerr << "rename failed" << endl;
				close(*(int*)pnewsock);
			}	
		} catch (exception const& e){
			cerr << "Couldn't write to file " << file_name << endl;
			msg = "ERROR;\n";
		}
		send(*(int *)pnewsock, msg.c_str(), msg.length(), 0);
		shutdown(*(int *)pnewsock, SHUT_WR);
	}


	else if (header.find("DOWNLOAD:") != string::npos){
		offset = 9;
		string file_name = header.substr(offset, header.find_first_of(";", offset) - offset);
		
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
		
		try{
			f.open(file_name, ios::in | ios::binary);
			f.read(fl, file_size);
			f.close();
		} catch (exception const& e){
			msg = "ERROR;\n";
			*fl = '\0';
			file_size = 0;
		}		
		
		int msg_size = msg.length();
		msg.append(fl, file_size);
		send(*(int *)pnewsock, msg.c_str(), msg_size + file_size, 0);
		delete [] fl;
	}
	close(*(int *)pnewsock);
	return NULL;
}

int main(int argc, char **argv){
	int sock;
	pthread_t thread;
	struct sockaddr_in srv;
	unsigned int PORT = 0;
	char * ptr = NULL;
	char * port = NULL;

	if (argc != 3){
		cerr << "There is only one parameter -p as port" << endl;
		return EXIT_FAILURE;
	}

	int c;
	opterr = 0;
	while ((c = getopt (argc, argv, "p:")) != -1)
		switch (c){
			case 'p':
				port = optarg;
				PORT = strtoul(port, &ptr, 10);
				if (*ptr != '\0'){
					cerr  << "Unrecognized port: " << port << endl;
					return EXIT_FAILURE;
				}
				break;
 			default:
				cerr  << "Unrecognized parameter: " << optarg << endl;
				return EXIT_FAILURE;
				break;
	  	}

	memset(&srv, 0, sizeof(sockaddr_in));
	srv.sin_family = AF_INET;
	srv.sin_addr.s_addr = INADDR_ANY;
	srv.sin_port = htons(PORT);	

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		cerr << "Couldn't create socket" << endl;
		return EXIT_FAILURE;
	}
	
	if (bind(sock, (struct sockaddr *)&srv, sizeof(srv)) == -1) {
		cerr << "Couldn't bind" << endl;
		return EXIT_FAILURE;
	}

	if (listen(sock, BACKLOG) == -1) {
		cerr << "Listen: failed" << endl;
		return EXIT_FAILURE;
	}

	while (1) {
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
				cerr << "Failed to create thread" << endl;
			}
		}
	}

	close(sock);

	return EXIT_SUCCESS;
}

