#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include <poll.h>
#include "comm.h"
#include "tcp_server.h"
#include <fcntl.h> // for open
#include "leds.h"

#define MAX 80
#define PORT 8080
#define SA struct sockaddr

static int sockfd, connfd, len;
static struct sockaddr_in servaddr, cli;

static pthread_t tcp_sr_receive;

/**
 * @brief Thread polling for the button press
 * 
 * @param arg 
 * @return void* 
 */
static void *tcp_sr_receive_thread (void * arg)
{
    struct pollfd sckts = {
        .fd = connfd,
        .events = POLLIN
    };

    while(1)
    {
        char in_cmd_buff[50];
        int num_bytes_rd = 0;
        if(poll(&sckts, 1, -1) < 0)
        {
            printf("error while polling\n");
            continue;
        }
        num_bytes_rd = read(connfd, in_cmd_buff, 50);
        if(num_bytes_rd < 0)
        {
            printf("Failed receiveing data\n");
            continue;
        }
        printf("Received %s from socket (%d bytes)\n", in_cmd_buff, num_bytes_rd);
        char * valid_command = strtok(in_cmd_buff, "\n");
        // Functions for parsing command and manipulating led status
        if(valid_command != NULL)
        {
            if(!strcmp(valid_command, "turn_on"))
            {
                leds_turn_on();
                comm_send_led_status("led_on");
            }
            else if(!strcmp(valid_command, "turn_off"))
            {
                leds_turn_off();
                comm_send_led_status("led_off");
            }
        }
        else
        {
            printf("Status not valid, continuing\n");
        }
    }
}

int tcp_server_init(void)
{  
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
   
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
   
    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        perror("socket bind failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");
   
    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");
    len = sizeof(cli);
   
    // Accept the data packet from client and verification
    connfd = accept(sockfd, (SA*)&cli, &len);
    if (connfd < 0) {
        printf("server accept failed...\n");
        exit(0);
    }
    else
        printf("server accept the client...\n");
    printf("socket number is %d\n", sockfd);
    if (pthread_create(&tcp_sr_receive, NULL, tcp_sr_receive_thread, NULL) != 0)
    {
        perror ("pthread_create(tcp_sr_receive_thread)");
        return -1;
    }
    return connfd;
}

void tcp_server_send(void)
{
    struct pollfd sckts = {
        .fd = connfd,
        .events = POLLOUT
    };
    char stat_buff[20] = {0};
    int num_bytes = 0;
    // Wait from status to arrive
    if(comm_receive_led_status(stat_buff) != 0)
    {
        return;
    }
    if(poll(&sckts, 1, -1) < 0)
    {
        printf("error while polling\n");
        return;
    }
    if(sckts.revents | POLLOUT)
    {
        printf("Writting %s to socket\n", stat_buff);
        num_bytes = write(connfd, stat_buff, (strlen(stat_buff) + 1));
        printf("Written %d bytes to socket\n", num_bytes);
    }
    else
    {
        printf("received events from sent socket: %d\n", sckts.revents);    
    }
}