#include "http.h"

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define  BUFF_SZ            1024
#define  MAX_REOPEN_TRIES   5

char recv_buff[BUFF_SZ];

char *generate_cc_request(const char *host, int port, const char *path){
	static char req[512] = {0};
	int offset = 0;
	
    //note that all paths should start with "/" when passed in
	offset += sprintf((char *)(req + offset),"GET %s HTTP/1.1\r\n", path);
	offset += sprintf((char *)(req + offset),"Host: %s\r\n", host);
	offset += sprintf((char *)(req + offset),"Connection: Keep-Alive\r\n");
	offset += sprintf((char *)(req + offset),"\r\n");

	printf("DEBUG: %s", req);
	return req;
}


void print_usage(char *exe_name){
    fprintf(stderr, "Usage: %s <hostname> <port> <path...>\n", exe_name);
    fprintf(stderr, "Using default host %s, port %d  and path [\\]\n", DEFAULT_HOST, DEFAULT_PORT); 
}

int reopen_socket(const char *host, uint16_t port) {
    int sock = 0;
    int num_reopen_tries = 0;
    while (num_reopen_tries < MAX_REOPEN_TRIES) {
        sock = socket_connect(host, port);
        if (sock > 0) {
            return sock;
        } else {
            num_reopen_tries++;
        }
    }

    return -1;
}

int server_connect(const char *host, uint16_t port){
    return socket_connect(host, port);
}

void server_disconnect(int sock){
    close(sock);
}

int submit_request(int sock, const char *host, uint16_t port, char *resource){
    int sent_bytes = 0; 

    const char *req = generate_cc_request(host, port, resource);
    int send_sz = strlen(req);

    // This is the initial send, this is where the send will fail with 
    // Keep-Alive if the server closed the socket, sent_bytes will have
    // the number of bytes sent, which should equal send_sz if all went
    // well.  If sent_bytes < 0, this indicates a send error, so we will
    // need to reconnect to the server.
    sent_bytes = send(sock, req, send_sz,0);

    //we have a socket error, perhaps the server closed it, lets try to reopen
    //the socket
    if (sent_bytes < 0) {
        int sock = reopen_socket(host, port);
        if (sock < 0) {
           perror("socket reopen");
        } else {
             sent_bytes = send(sock, req, send_sz,0);
        }
        
    }

    //This should not happen, but just checking if we didnt send everything and 
    //handling appropriately 
    if(sent_bytes != send_sz){
        if(sent_bytes < 0)
            perror("send failed after reconnect attempt");
        else
            fprintf(stderr, "Sent bytes %d is not equal to sent size %d\n", sent_bytes, send_sz);
        
        close(sock);
        return -1;
    }

    int bytes_recvd = 0;    //used to track amount of data received on each recv() call
    int total_bytes = 0;    //used to accumulate the total number of bytes across all recv() calls
    
    //do the first recv
    bytes_recvd = recv(sock, recv_buff, sizeof(recv_buff),0);
    if(bytes_recvd < 0) {
        perror("initial receive failed");
        close(sock);
        return -1;
    }

    //printf("%.*s", bytes_recvd, recv_buff);


    //remember the first receive we just did has the HTTP header, and likely some body
    //data.  We need to determine how much data we expect

    printf("Got to header len\n");
    int header_len = get_http_header_len(recv_buff, bytes_recvd);     //change this to get the header len as per the directions above
    if (header_len < 0) {
        close(sock);
        return -1;
    }

    printf("Got over header length\n");

    int content_len = get_http_content_len(recv_buff, header_len);    //Change this to get the content length
    printf("Got passed content length\n");
    //--------------------------------------------------------------------------------
    // TODO:  Make sure you understand the calculations below
    //
    // You do not have to write any code, but add to this comment your thoughts on 
    // what the following 2 lines of code do to track the amount of data received
    // from the server
    //
    // YOUR ANSWER: Initial data represents the amount of data recieved thus far by the client, represented by recieved bytes minus header lenght, which is current payload
    // Bytes remaining represents how many bytes are left to be read in by the client, where content length is the total length of what will be recieved, minus what has already been recieved
    //--------------------------------------------------------------------------------
    int initial_data =  bytes_recvd - header_len;
    int bytes_remaining = content_len - initial_data;


    //This loop keeps going until bytes_remaining is essentially zero, to be more
    //defensive an prevent an infinite loop, i have it set to keep looping as long
    //as bytes_remaining is positive
    while(bytes_remaining > 0){
        bytes_recvd = recv(sock, recv_buff, sizeof(bytes_recvd), 0); // replace with a valid recv(...); call
        // bytes_recvd = recv(sock, bytes_recvd, sizeof(bytes_recvd), 0); // replace with a valid recv(...); call
        //You can uncomment out the fprintf() calls below to see what is going on

        fprintf(stdout, "%.*s", bytes_recvd, recv_buff);
        total_bytes += bytes_recvd;
        //fprintf(stdout, "remaining %d, received %d\n", bytes_remaining, bytes_recvd);
        bytes_remaining -= bytes_recvd;
    }

    fprintf(stdout, "\n\nOK\n");
    fprintf(stdout, "TOTAL BYTES: %d\n", total_bytes);

    //processed the request OK, return the socket, in case we had to reopen
    //so that it can be used in the next request

    //---------------------------------------------------------------------------------
    // TODO:  Documentation
    //
    // You dont have any code to change, but explain why this function, if it gets to this
    // point returns an active socket.
    //
    // YOUR ANSWER:  the socket is active at this point because if we have gotten to this point, that means we have opened the socket,
    // and we have read all of the bytes from the socket using recv() successfully. So the socket was just still open and recieving bytes.
    //--------------------------------------------------------------------------------
    return sock;
}

int main(int argc, char *argv[]){
    clock_t start, end;
    double time;
    start = clock();
    int sock;

    const char *host = DEFAULT_HOST;
    uint16_t   port = DEFAULT_PORT;
    char       *resource = DEFAULT_PATH;
    int        remaining_args = 0;

    //YOU DONT NEED TO DO ANYTHING OR MODIFY ANYTHING IN MAIN().  MAKE SURE YOU UNDERSTAND
    //THE CODE HOWEVER
    sock = server_connect(host, port);

    if(argc < 4){
        print_usage(argv[0]);
        //process the default request
        submit_request(sock, host, port, resource);
	} else {
        host = argv[1];
        port = atoi(argv[2]);
        resource = argv[3];
        if (port == 0) {
            fprintf(stderr, "NOTE: <port> must be an integer, using default port %d\n", DEFAULT_PORT);
            port = DEFAULT_PORT;
        }
        fprintf(stdout, "Running with host = %s, port = %d\n", host, port);
        remaining_args = argc-3;
        for(int i = 0; i < remaining_args; i++){
            resource = argv[3+i];
            fprintf(stdout, "\n\nProcessing request for %s\n\n", resource);
            sock = submit_request(sock, host, port, resource);
        }
    }

    server_disconnect(sock);
    end = clock();

    time = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Program runtime: %f seconds\n", time);
}