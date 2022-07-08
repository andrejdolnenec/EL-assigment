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
#include "tcp_client.h"


#define PORT 8080
#define SA struct sockaddr

#define CMD_TIMEOUT_S   5

#define TEST_IP     "127.0.0.1"     // This is test ip, remove it when testing with real ip

static char ip_addr[80] = {0};

static pthread_t tcp_cl_send;
static pthread_t tcp_cl_receive;

static timer_t timer_id;

static int sockfd;
static struct sockaddr_in servaddr;

static void expired_cmd_callback(union sigval timer_data)
{
    printf("Didn't receive status update since the latest command...\n");
    printf("Resending...\n");
    comm_resend_command();
}

/**
 * @brief Open connection to server (port is hardcoded 8080)
 * 
 * @param ip_addr the ip address to which the host will connect
 * @return int 
 */
static int tcp_client_connect(char * ip_addr)
{
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        return -1;
    }
    printf("Socket successfully created.. %d\n", sockfd);
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    printf("Assigning IP: %s, port: %d\n", ip_addr, PORT);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip_addr);
    servaddr.sin_port = htons(PORT);

    // connect the client socket to server socket
    int ret = connect(sockfd, (SA*)&servaddr, sizeof(servaddr));
    if (ret != 0) {
        printf("connection with the server failed... ret = %d\n", ret);
        return -1;
    }

    printf("connected to the server..\n");
    return 0;
}

/**
 * @brief Close the socket
 */
static void tcp_client_disconnect(void)
{
    close(sockfd);
}

/**
 * @brief Thread sending commands to the server
 * 
 * @param arg 
 * @return void* 
 */
static void *tcp_cl_send_thread (void * arg)
{
    struct pollfd sckts = {
        .fd = sockfd,
        .events = POLLOUT
    };

    /* specify start delay and interval
     * it_value and it_interval must not be zero */

    struct itimerspec its = {   .it_value.tv_sec  = CMD_TIMEOUT_S,
                                .it_value.tv_nsec = 0,
                                .it_interval.tv_sec  = CMD_TIMEOUT_S,
                                .it_interval.tv_nsec = 0
                            };
    while(1)
    {
        char cmd_buff[20] = {0};
        int num_bytes = 0;
        // Wait from command from the main task
        if(comm_receive_command(cmd_buff) != 0)
        {
            continue;
        }
        if(poll(&sckts, 1, -1) < 0)
        {
            printf("error while polling\n");
            continue;
        }
        if(sckts.revents | POLLOUT)
        {
            printf("Writting %s to socket\n", cmd_buff);
            num_bytes = write(sockfd, cmd_buff, (strlen(cmd_buff) + 1));
            printf("Written %d bytes to socket\n", num_bytes);
            // Start the expiration timer after sending command
            timer_settime(timer_id, 0, &its, NULL);
        }
        else
        {
            printf("received events from sent socket: %d\n", sckts.revents);    
        }

    }
}

/**
 * @brief Thread receiving messages from the server
 * 
 * @param arg 
 * @return void* 
 */
static void *tcp_cl_receive_thread (void * arg)
{
    struct pollfd sckts = {
        .fd = sockfd,
        .events = POLLIN
    };

    struct itimerspec its = {   .it_value.tv_sec  = 0,
                                .it_value.tv_nsec = 0,
                                .it_interval.tv_sec  = 0,
                                .it_interval.tv_nsec = 0
                            };
    while(1)
    {
        char in_stat_buff[50];
        int num_bytes_rd = 0;
        if(poll(&sckts, 1, -1) < 0)
        {
            printf("error while polling\n");
            continue;
        }
        num_bytes_rd = read(sockfd, in_stat_buff, 50);
        if(num_bytes_rd < 0)
        {
            printf("Failed receiveing data\n");
            continue;
        }
        printf("Received %s from socket (%d bytes)\n", in_stat_buff, num_bytes_rd);
        // Disarm the timer when a status is received
        timer_settime(timer_id, 0, &its, NULL);
        char * valid_status = strtok(in_stat_buff, "\n");
        if(valid_status != NULL)
        {
            comm_send_status(valid_status);
        }
        else
        {
            printf("Status not valid, continuing\n");
        }
    }
}

/**
 * @brief Initialize the tcp client, connect to server
 * 
 * @param ip_addr the ip address to which to connect
 * @return int 0 on success, -1 on failure
 */
int tcp_client_init(char * ip_addr)
{
    printf("Starting tcp_client\n");
    if(tcp_client_connect(ip_addr) != 0)
    {
        return -1;
    }

    // Starting two tcp threads, one for receive and one for sending data
    if (pthread_create(&tcp_cl_receive, NULL, tcp_cl_receive_thread, NULL) != 0)
    {
        perror ("pthread_create(tcp_cl_receive_thread)");
        return -1;
    }
    if (pthread_create(&tcp_cl_send, NULL, tcp_cl_send_thread, NULL) != 0)
    {
        perror ("pthread_create(tcp_cl_send_thread)");
        return -1;
    }

    /*  sigevent specifies behaviour on expiration  */
    struct sigevent sev = { 0 };

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = &expired_cmd_callback;
    sev.sigev_value.sival_ptr = NULL;

    /* create timer */
    if(timer_create(CLOCK_REALTIME, &sev, &timer_id))
    {
        perror("Timer create error\n");
    }

    return 0;
}

/**
 * @brief Close the socket oppened from the connection
 */
void tcp_client_deinit(void)
{
    timer_delete(timer_id);
    tcp_client_disconnect();
}