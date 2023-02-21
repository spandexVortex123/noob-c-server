#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>

#define MAX_SIZE 65535
#define ACCESS_LOG "access.log"
#define _200_HEADER "HTTP/1.1 200 OK\r\nServer: Not Apache Server!\r\nContent-Type: text/html\r\nContent-Length: "
#define _404_HEADER "HTTP/1.1 404 NOT FOUND\r\nServer: Not Apache Server!\r\nContent-Type: text/html\r\nContent-Length: "

void handle_get(const char* path, int socket_to_write);
void handle_post(const char* path, int socket_to_write);

int get_size(const char* ptr) {
	int i = 0;
	while(ptr[i++] != '\0');
	return i - 1;
}

// writing log
void write_log(char* message) {
	FILE* fp;
	fp = fopen(ACCESS_LOG, "a");
	fprintf(fp, "%s", message);
	fclose(fp);
}

// parse request
void parse_request(char* request_data, char* client_ip, int client_port, int new_sock) {
	//printf("--------%s\n---------", request_data);
	// break request into each lines
	int i=0;
	/*
	while((request_data[i] != '\0' || request_data[i] != EOF) && i<500) {
		printf(" %c : %d\n", request_data[i], (int)request_data[i]);
		i++;
	}
	*/

	char** lines = malloc(sizeof(char*) * 50); // assuming number of lines per request will not exceed 50 lines

	char* line = strtok(request_data, "\r\n");
	while(line != NULL) {
		lines[i++] = line;
		line = strtok(NULL, "\r\n");
	}
	
	// puts("begin");
	// int j=0;
	// while(lines[j] != NULL) {
	// 	char* l = lines[j];
	// 	printf("%s\n", l);
	// 	j++;
	// }
	// puts("end");

	// printf("size of first line %d\n", size);
	// i=0;
	// while((lines[0][i] != '\0' || lines[0][i] != 10) || i<20) {
	// 	printf(" %c : %d\n", lines[0][i], (int)lines[0][i]);
	// 	i++;
	// }

	// break first line by space
	int k=0;
	char** first_line_words = malloc(sizeof(char*) * 10);
	char* first_line_word = strtok(lines[0], " ");
	// printf("%s\n", lines[0]);
	while(first_line_word != NULL) {
		first_line_words[k++] = first_line_word;
		first_line_word = strtok(NULL, " ");
	}

	// puts("words begin");
	// int l=0;
	// while(words[l] != NULL) {
	// 	printf("%s\n", words[l]);
	// 	l++;
	// }
	// puts("words end");	

	// write log
	char* log_buffer = malloc(sizeof(char) * 200);
	sprintf(log_buffer, "%s|%d|%s|%s\n", client_ip, client_port, first_line_words[0], first_line_words[1]);
	write_log(log_buffer);
	free(log_buffer);

	printf("Method : %s\n", first_line_words[0]);

	if(!strcmp(first_line_words[0], "GET")) {
		handle_get(first_line_words[1], new_sock);
	} else if(!strcmp(first_line_words[0], "POST")) {
		handle_post(first_line_words[1], new_sock);
	}
	
	
	
	free(first_line_words);
	free(lines);
}

void handle_get(const char* path, int socket_to_write) {
	char* buffer = calloc(MAX_SIZE, sizeof(char));
	if(!strcmp(path, "/") || !strcmp(path, "/index.html")) {
		FILE* fp;
		fp = fopen("index.html", "r");
		fseek(fp, 0L, SEEK_END);
		int file_size = ftell(fp);
		rewind(fp);
		puts("in handle request");
		puts(path);

		// to convert int to char*
		char* size_buffer = calloc(10, sizeof(char));
		sprintf(size_buffer, "%d", file_size);

		printf("file size %s\n", size_buffer);

		char* file_buffer = calloc(file_size, sizeof(char));
		fread(file_buffer, 1, file_size, fp);

		//printf("****************%s************\n", file_buffer);

		buffer = strcat(buffer, _200_HEADER);
		buffer = strcat(buffer, size_buffer);
		buffer = strcat(buffer, "\r\n\r\n");
		buffer = strcat(buffer, file_buffer);

		// printf("response \n\n------------------------\n\n%s\n\n", buffer);

		// write to socket
		int response_size = write(socket_to_write, buffer, MAX_SIZE);
		printf("[!] Written %d bytes back to request\n", response_size);
		close(socket_to_write);

		free(size_buffer);
		free(file_buffer);
		//printf("response \n\n------------------------\n\n%s\n\n", buffer);
	} else {
		int size = get_size(path);
		char* file_name = malloc(sizeof(char) * (size - 1));

		int i=1,j=0;
		while(path[i] != '\0') {
			file_name[j++] = path[i];
			i++;
		}

		printf("File name: %s\n", file_name);

		// check file if exists or not
		// if(access(file_name, F_OK) != 0) {
		// 	// return 404
		// 	if()
		// }

	}
}

void handle_post(const char* path, int socket_to_write) {

}


int main() {

	// create socket
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0) {
		perror("socket creation error: ");
		exit(EXIT_FAILURE);
	}

	// prepare struct sockaddr_in
	struct sockaddr_in server, client;

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8001);

	// bind
	int bind_result = bind(sock, (struct sockaddr*)&server, sizeof(server));
	if(bind_result < 0) {
		perror("bind error ");
		exit(EXIT_FAILURE);
	}

	// listen 
	listen(sock, 3);
	printf("server listening\n");

	// infinite while loop
	while(1) {
		// get connection details
		int client_len = sizeof(client);
		printf("%d\n", client_len);
		int new_sock = accept(sock, (struct sockaddr*)&client, (socklen_t*)&client_len);

		// check error
		if(new_sock < 0) {
			perror("accept error ");
			close(sock);
			exit(EXIT_FAILURE);
		}

		// client information
		char* client_ip = inet_ntoa(client.sin_addr);
		int client_port = ntohs(client.sin_port);

		// read from socket
		// puts("going to ioctl");
		// ioctl(new_sock, FIONREAD, &asize);
		char* buffer = malloc(MAX_SIZE);
		// recv data
		int recved_size = read(new_sock, buffer, MAX_SIZE);
		if(recved_size < 0) perror("read ");
		parse_request(buffer, client_ip, client_port, new_sock);
		// printf("dataaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n\n%s\n\n", response_data);
		// parse and respond
		char message[] = "sending data back\n";
		const char hello[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nHello world!";
		// int sent_bytes = write(new_sock, response_data, MAX_SIZE);
		
		free(buffer);

		close(new_sock);

	}

	return 0;
}