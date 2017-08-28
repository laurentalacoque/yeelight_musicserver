#include <stdio.h>
#include <string.h>    //strlen
#include <stdlib.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h> //for threading , link with lpthread

#define SERVER_PORT (54321)
//the thread function
void *connection_handler(void *);
void *producer_handler(void *);
char global_message[1000];
int new_count=0;
pthread_mutex_t message_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t message_signal = PTHREAD_COND_INITIALIZER;

void* produce(void *arg){
	while(1) {
		pthread_mutex_lock(&message_mtx);
        new_count +=1;
        sprintf(global_message,"{\"id\":1, \"method\":\"set_scene\", \"params\":[\"ct\", 5400, %d]}\r\n",new_count); 
		if(new_count >=100) new_count=0;
		pthread_cond_broadcast(&message_signal);
		pthread_mutex_unlock(&message_mtx);
		usleep(100000);
	}
    return NULL;
}
/*
void consume(void){
	pthread_mutex_lock(&message_mtx);
	while(1) {
		pthread_cond_wait(&message_signal,&message_mtx);
		printf("consumer : new count: %d\n",new_count);
	}
	pthread_mutex_unlock(&message_mtx);
}
*/

int main(int argc , char *argv[])
{
	int socket_desc , client_sock , c , *new_sock;
	struct sockaddr_in server , client;
	pthread_t pthread[3];
	if( pthread_create( &(pthread[0]) , NULL ,  produce , NULL) < 0)
	{
		perror("could not create thread");
		return 1;
	}

	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
	puts("Socket created");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( SERVER_PORT );

	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");

	//Listen
	listen(socket_desc , 3);

	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);


	//Accept and incoming connection
	/*puts("Waiting for incoming connections...");
	  c = sizeof(struct sockaddr_in);*/

	while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
	{
		puts("Connection accepted");

		pthread_t sniffer_thread;
		new_sock = malloc(sizeof(int*));
		*new_sock = client_sock;

		if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
		{
			perror("could not create thread");
			return 1;
		}

		//Now join the thread , so that we dont terminate before the thread
		//pthread_join( sniffer_thread , NULL); // was commented before
		puts("Handler assigned");
	}

	if (client_sock < 0)
	{
		perror("accept failed");
		return 1;
	}

	return 0;
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
	//Get the socket descriptor
	int sock = *(int*)socket_desc;
	int write_size;
	pthread_mutex_lock(&message_mtx);
	while(1) {
		pthread_cond_wait(&message_signal,&message_mtx);
		//send new message
		printf("sending %s...\n",global_message);
		write_size=write(sock,global_message,strlen(global_message));
		if (write_size <0) {
			fprintf(stderr,"Error, exiting thread\n");
			//Free the socket pointer
			pthread_mutex_unlock(&message_mtx);
			free(socket_desc);
			close(sock);
			pthread_exit(NULL); 
			return;
		}
	}
	pthread_mutex_unlock(&message_mtx);

}
