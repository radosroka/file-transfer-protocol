#include <iostream>
#include <string>

#include <cstring> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>

#define PORT	"8000"
#define BACKLOG	 50

using namespace std;

const int BUFFER_SIZE = 2048;

void * handle(void *pnewsock)
{
	/* send(), recv(), close() */
	cout << "I'm in thread" << endl;
	
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

		cout << data;

	return NULL;
}

int main(void)
{
	int sock;
	pthread_t thread;
	struct addrinfo hints, *res;
	int reuseaddr = 1; /* True */

	/* Get the address info */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(NULL, PORT, &hints, &res) != 0) {
		perror("getaddrinfo");
		return 1;
	}

	/* Create the socket */
	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sock == -1) {
		perror("socket");
		return 1;
	}

	/* Enable the socket to reuse the address */
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int)) == -1) {
		perror("setsockopt");
		return 1;
	}

	/* Bind to the address */
	if (bind(sock, res->ai_addr, res->ai_addrlen) == -1) {
		perror("bind");
		return 0;
	}

	freeaddrinfo(res);

	/* Listen */
	if (listen(sock, BACKLOG) == -1) {
		perror("listen");
		return 0;
	}

	/* Main loop */
	while (1) {
		printf("In while\n");
		socklen_t size = sizeof(struct sockaddr_in);
		struct sockaddr_in their_addr;
		int newsock = accept(sock, (struct sockaddr*)&their_addr, &size);
		if (newsock == -1) {
			perror("accept");
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

	return 0;
}

