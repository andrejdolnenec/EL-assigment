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
#include <signal.h>
#include "comm.h"
#include "tcp_client.h"


static char ip_addr[80] = {0};

/**
 * @brief Just a simple function for ip validation
 * 
 * @param ip_addr The ip address string
 * @return int 0 on success, -1 on error
 */
static int validate_ip(char * ip_addr)
{
    if(ip_addr == NULL)
    {
        printf("Error: The inputed ip address is not valid ip_addr is NULL\n");
        return -1;
    }
    char * token = NULL;
    char cpy[strlen(ip_addr) + 1];
    strcpy(cpy, ip_addr);
    int sub_seg = 0;
    int num;
    int tot_len = 0;

    // Run through all substrings delimited by '.' and check them
    do
    {
        // Get the current token(sub string)
        token = strtok(&cpy[tot_len], ".");
        printf("%s \n", token);
        // Count it
        sub_seg ++;
        // Convert it to number
        num = atoi(token);
        // Check if the integer is okay
        if(num > 255 || num < 0)
        {
            printf("Error: The inputed ip address is not valid, error in substring %s \n", token);
            return -1;
        }
        // Calculate the total length of chars so far (add 1 'cause of the '.')
        tot_len = tot_len + strlen(token) + 1;
        if(tot_len >= strlen(ip_addr))
        {
            // Total length reaches the one of the full string
            break;
        }
        // Exit when no more sub strings
    } while (token != NULL);

    if(sub_seg != 4)
    {
        printf("Error: The inputed ip address is not valid, not 4 sub segments\n");
        return -1;
    }

    printf("IP is valid \n");
    return 0;
}

/**
 * @todo Implemnt deinit functions, comm_deinit and tcp_client deinit
 */
int main(int argc, char * argv[])
{
    pid_t childPID;
    while (1)
    {
        printf("Enter ip address of the r-pi server: \n");
        printf("or enter 'exit' to exit\n");
        printf("For testing enter 127.0.0.1\n");
        fscanf(stdin, "%s", ip_addr);
        // Parse the input
        if(!strcmp(ip_addr, "exit"))
        {
            printf("exiting...\n\n");
            exit(0);
        }
        printf("Validating ip address\n");
        if(validate_ip(ip_addr))
        {
            printf("Retry the ip\n");
            continue;
        }

        if(comm_init() != 0)
        {
            printf("comm_init failed\n");
            exit(-1);
        }

        if(tcp_client_init(ip_addr) != 0)
        {
            printf("tcp_client_init failed\n");
            exit(-1);
        }

        break;
    }
    // The host should be connected by now
    childPID = fork();
    if(childPID < 0)
    {
        printf("Error in forking\n");
        exit(-1);
    }
    if(childPID == 0)// This is the child which will be the waiting for status process
    {
        while(1)
        {
            char status[50];
            if(comm_receive_status(status) != 0)
            {
                printf("Error in comm_receive_status\n");
                continue;
            }
            printf("in child Status received : %s\n", status);
        }
    }
    else // This is the main proccess
    {
        char in_cmd[20];
        while (1)
        {
            printf("Write command\n");
            fscanf(stdin, "%s", in_cmd);
            if(!strcmp(in_cmd, "exit"))
            {
                break;
            }
            comm_send_command(in_cmd);
        }
        printf("exiting ...\n");
        tcp_client_deinit();
        comm_deinit();
        // Other threads terminate by default when exiting from main
        // Kill the child proccess
        if (kill(childPID, SIGKILL) == -1) {
            perror("kill");
            exit(EXIT_FAILURE);
        }
        exit(0);
    }

    exit(0);
}