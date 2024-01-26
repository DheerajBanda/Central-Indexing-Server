#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define PORT 8080
#define MAX_QUEUE_SIZE 10
#define NUM_THREADS 4

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

void* handle_request(void* arg) {
	ThreadInfo* thread_info = (ThreadInfo*)arg;

		Request* request = dequeue_request();
                
                int valread;
		char buffer[1024] = { 0 };
          	char* m1 = "data server connection estblished\n";
		char* invalid = "Invalid Response\nDo you want to continue(Y/N): ";
		char* exit1 = "Connection Closed";
		char* m2 = "file not found\nDo you want to continue for another task(Y/N): ";

		while (1) {
                send(request->client_socket, m1, strlen(m1), 0);
                printf("Message 1 sent\n");
		valread = read(request->client_socket, buffer, 1024);
                printf("%s\n", buffer);
		char command[1047];
		snprintf(command, sizeof(command), "find . -type f -name '%s'", buffer);
		FILE* pipe = popen(command, "r");
		if (pipe == NULL ) {
			printf("Failed to execute command\n");
			break;
		}
		char path[1047];
		if (fgets(path, sizeof(path), pipe) == NULL) {
			printf("File not found\n");
			pclose(pipe);
			break;
		}
		size_t len = strlen(path);
		if (len > 0 && path[len-1] == '\n') {
			path[len-1] = '\0';
		}
		pclose(pipe);
		FILE* file = fopen(path, "rb");
   		if (file == NULL) {
       		perror("File opening failed");
    		}else {
			printf("file opened\n");
		}

    		char buffer2[1024];
    		size_t bytes_read;
    		while ((bytes_read = fread(buffer2, 1, sizeof(buffer2), file)) > 0) {
        	if (send(request->client_socket, buffer2, bytes_read, 0) == -1) {
            		perror("Failed to send file data");
            		fclose(file);
			break;
        		} else {
				printf("buffer2 is %s\n", buffer2);
				printf("File successfully sent\n");
 		}			
			
		}

		fclose(file);

		send(request->client_socket, "Connection Closed", strlen("Connection Closed"), 0);	
                  
		printf("Thread %d handled a request from client %d.\n", thread_info->thread_num, request->client_socket);

		close(request->client_socket);
		free(request);

	return NULL;
}
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

