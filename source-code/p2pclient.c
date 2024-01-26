#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>

#define NUM_SERVERS 3
#define PORT 9000
#define PORT1 8080


typedef struct {
	char* ip;
	int port;
} ServerInfo;

void handle_server(ServerInfo* server_info) {
  

    char* server_ip = server_info->ip;
    int server_port = server_info->port;

    printf("%s\n", server_ip);
    printf("%i\n", server_port);

    int status, valread, client_fd;
    struct sockaddr_in serv_addr;
    char* hello = "Hello from client";
    char buffer[4096] = { 0 };
    char buffer1[4096] = { 0 };
    char IP[15];
    DIR *directory;
    struct dirent *entry;
    char* exit1 = "\nConnection Closed\n";    

    int client_socket;
    struct sockaddr_in server_address;

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0) {
        perror("Invalid address/Address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }


    do {
                valread = read(client_socket, buffer1, sizeof(buffer));
                printf("%s\n", buffer1);
                if(strcmp(buffer1, exit1) == 0) {
                        break;
                }
		if (strcmp(buffer1, "path") == 0) {
			char tempb[10];
			char path[256];
			printf("entered path code\n");
			scanf("%s", path);
			printf("path is: %s\n", path);
			directory = opendir(path);
			printf("executed opendir(path)\n");
			if(directory == NULL) {
				perror("Unable to open directory");
			}
			while ((entry = readdir(directory))) {
				if(entry->d_type == DT_REG) {
					printf("file name is: %s\n", entry->d_name);
					send(client_socket, entry->d_name, strlen(entry->d_name), 0);
					read(client_socket, tempb, sizeof(tempb));
						while(strcmp(tempb, "done") != 0) {
							continue;
						}
				}
			}
			printf("task completed done\n");
			closedir(directory);
			break;
		}
		if (strncmp(buffer1, "IP addr", 7) == 0) {
			scanf("%s", buffer);
                        if (strcmp(buffer, "y") == 0|| strcmp(buffer, "Y") == 0) {
				char IP[16];
				int i;
				int k = 0;
				printf("buffer1 is %s\n", buffer1); 
				for(i = 9; i < strlen(buffer1) && buffer1[i] != ' '; i++) {
					IP[k] = buffer1[i];
					k++;
				}
				ServerInfo newServer;
				newServer.ip = IP;
				newServer.port = PORT1;
                                send(client_fd, "N", strlen("N"), 0);
                                close(client_socket);
                                printf("Connecting to the server IP addr: %s\n", IP);
				handle_server(&newServer);
				
                        }else{
				close(client_socket);
			}
		}
		if (strncmp(buffer1, "data server", 11) == 0) {
			printf("Enter the file name: \n");
			scanf("%s", buffer);
			send(client_socket, buffer, strlen(buffer), 0);
   			FILE* received_file = fopen(buffer, "w+");
   	 		if (received_file == NULL) {
        			perror("File opening failed");
        			close(client_socket);
    			}

    			char buffer2[1024];
    			size_t bytes_received;
    			while ((bytes_received = read(client_socket, buffer2, sizeof(buffer2))) > 0&& strcmp(buffer2, "Connection Closed") != 0) {
        			fwrite(buffer2, 1, bytes_received, received_file);
			}

    			printf("done\n");
    			fclose(received_file);
			break;

		}
		scanf("%s", buffer);
                send(client_socket, buffer, strlen(buffer), 0);
                memset(buffer1, 0, sizeof(buffer1));
                memset(buffer, 0, sizeof(buffer));
	} while(1);

    close(client_socket);
}


int main() {

    ServerInfo server_ips;

    server_ips.ip = "192.168.100.4";
    server_ips.port = PORT;

    handle_server(&server_ips);

    return 0;
}
