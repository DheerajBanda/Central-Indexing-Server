#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define PORT 9000
#define MAX_QUEUE_SIZE 10
#define NUM_THREADS 6

typedef struct {
	int client_socket;
	char IP[15];
} Request;

typedef struct {
	pthread_t thread_id;
	int thread_num;
} ThreadInfo;

typedef struct {
	Request* requests[MAX_QUEUE_SIZE];
	int front;
	int rear;
	int count;
	pthread_mutex_t mutex;
	pthread_cond_t not_empty;
	pthread_cond_t not_full;
} RequestQueue;

RequestQueue request_queue;

void init_request_queue() {
	request_queue.front = 0;
	request_queue.rear = -1;
	request_queue.count = 0;
	pthread_mutex_init(&request_queue.mutex, NULL);
	pthread_cond_init(&request_queue.not_empty, NULL);
	pthread_cond_init(&request_queue.not_full, NULL);
}

void enqueue_request(Request* request) {
	pthread_mutex_lock(&request_queue.mutex);

	while(request_queue.count >= MAX_QUEUE_SIZE) {
		pthread_cond_wait(&request_queue.not_full, &request_queue.mutex);
	}

	request_queue.rear = (request_queue.rear + 1) % MAX_QUEUE_SIZE;
	request_queue.requests[request_queue.rear] = request;
	request_queue.count++;

	pthread_cond_signal(&request_queue.not_empty);
	pthread_mutex_unlock(&request_queue.mutex);
}

Request* dequeue_request() {
	pthread_mutex_lock(&request_queue.mutex);

	while (request_queue.count <= 0) {
		pthread_cond_wait(&request_queue.not_empty, &request_queue.mutex);
	}

	Request* request = request_queue.requests[request_queue.front];
	request_queue.front = (request_queue.front +1) % MAX_QUEUE_SIZE;
	request_queue.count--;

	pthread_cond_signal(&request_queue.not_full);
	pthread_mutex_unlock(&request_queue.mutex);

	return request;
}

typedef struct {
    char filename[256];  // Assuming a maximum filename length of 255 characters
    char location[256];  // Assuming a maximum location length of 255 characters
} FileInfo;

void append_file_info(const char* filename, const char* location) {
    FILE* file = fopen("file_info.txt", "a"); // Open the file in append mode
    if (file == NULL) {
	    FILE* file = fopen("file_info.txt", "w");
	    if (file == NULL) {
        	perror("Failed to open file for writing");
		return;
	    }
        return;
    }

    fprintf(file, "%s %s\n", filename, location);

    fclose(file);
}

bool search_file_location(const char* filename, char* location) {
    FILE* file = fopen("file_info.txt", "r"); // Open the file in read mode
    if (file == NULL) {
        perror("Failed to open file for reading");
        return false;
    }

    char line[512]; // Assuming a maximum line length of 511 characters
    while (fgets(line, sizeof(line), file) != NULL) {
        char file_entry[256], location_entry[256];
        if (sscanf(line, "%255s %255s", file_entry, location_entry) == 2) {
            if (strcmp(file_entry, filename) == 0) {
                strcpy(location, location_entry);
                fclose(file);
                return true;
            }
        }
    }

    fclose(file);
    return false;
}


void* handle_request(void* arg) {
	ThreadInfo* thread_info = (ThreadInfo*)arg;

	while (1) {	
	Request* request = dequeue_request();
                
                int valread;
		char buffer[1024] = { 0 };
          	char* m1 = "Connection established\nTasks\n1. Register a file\n2. search for a file\n3. exit";
		char* m1_1 = "What is the file name?";
		char* invalid = "Invalid Response\nDo you want to continue(Y/N): ";
		char* exit = "\nConnection Closed\n";
		char* path = "path";
		char* m2 = "file not found\nDo you want to continue for another task(Y/N): ";

		while (1) {
                send(request->client_socket, m1, strlen(m1), 0);
                printf("Message 1 sent\n");
		valread = read(request->client_socket, buffer, 1024);
                printf("%s\n", buffer);
			
		if (strcmp(buffer, "1") == 0) {
			memset(buffer, 0, sizeof(buffer));
			send(request->client_socket, path, strlen(path), 0);
			printf("tasks 1 message sent\n");
			do {
				int r = 0;
				ssize_t bytes_r = recv(request->client_socket, buffer, sizeof(buffer), 0);
				if (bytes_r <= 0) {
					break;
				}
				append_file_info(buffer, request->IP);
				memset(buffer, 0, sizeof(buffer));
				send(request->client_socket, "done", strlen("done"), 0);
			} while(1);
			printf("files added\n");
			break;
		}
		else if (strcmp(buffer, "2") == 0) {
			memset(buffer, 0, sizeof(buffer));
			char location[256];
			send(request->client_socket, m1_1, strlen(m1_1), 0);
			printf("task 2 message sent\n");
			read(request->client_socket, buffer, 1024);
			if (search_file_location(buffer, location)) {
				char location1[256] = "IP addr: ";
				strcat(location, " \nDo you want to connect to the server(Y/N): ");
				strcat(location1, location);
                 		send(request->client_socket, location1, strlen(location1), 0);
                	} else {
                    	 	send(request->client_socket, m2, strlen(m2), 0);
                	}
			read(request->client_socket, buffer, 1024);
                        if(strcmp(buffer, "Y") == 0 || strcmp(buffer, "y") == 0) {
                                memset(buffer, 0, sizeof(buffer));
				continue;
                        } else {
				memset(buffer, 0, sizeof(buffer));
                                break;
                        }

		}else if (strcmp(buffer, "3") == 0) {
			memset(buffer, 0, sizeof(buffer));
			break;
		}else {
			memset(buffer, 0, sizeof(buffer));
			send(request->client_socket, invalid, strlen(invalid), 0);
			read(request->client_socket, buffer, 1024);
			if(strcmp(buffer, "Y") == 0 || strcmp(buffer, "y") == 0) {
				memset(buffer, 0, sizeof(buffer));
				continue;
			} else {
				memset(buffer, 0, sizeof(buffer));
				break;
			}
		}

	}
			

		printf("Thread %d handled a request from client %d.\n", thread_info->thread_num, request->client_socket);

		send(request->client_socket, exit, strlen(exit), 0);
		close(request->client_socket);
		free(request);
		printf("closed and done\n");

	}
	return NULL;
}


int main() {
	int server_socket, client_socket;
	struct sockaddr_in server_addr, client_addr;
	int opt = 1;
	socklen_t client_len = sizeof(client_addr);

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}


	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
		perror("Binding failed");
		close(server_socket);
		exit(EXIT_FAILURE);
	}

	if (listen(server_socket, 3) == -1) {
		perror("Listening failed");
		close(server_socket);
		exit(EXIT_FAILURE);
	}

	ThreadInfo threads[NUM_THREADS];
	for (int i = 0; i < NUM_THREADS; i++) {
		threads[i].thread_num = i;
		if (pthread_create(&(threads[i].thread_id), NULL, handle_request, &(threads[i])) != 0) {
			perror("Thread creation failed");
			exit(EXIT_FAILURE);
		}
	}

	init_request_queue();

	printf("Listening\n");

	while (1) {
		client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
		
		if (client_socket == -1) {
			perror("Accepting client connection failed");
			close(server_socket);
			exit(EXIT_FAILURE);
		}else {
			printf("accept done\n");
		}


		printf("IP address is: %s\n", inet_ntoa(client_addr.sin_addr));


		Request* request = (Request*)malloc(sizeof(Request));
        	if (request == NULL) {
                	perror("Failed to allocate memory for request");
                	close(client_socket);
       		 }
       		request->client_socket = client_socket;
		strcpy(request->IP , inet_ntoa(client_addr.sin_addr));
          	enqueue_request(request);
	}


	close(server_socket);
	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i].thread_id, NULL);
	}

	return 0;
}

