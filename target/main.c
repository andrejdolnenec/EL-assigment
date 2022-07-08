#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include "tcp_server.h"
#include "button.h"
#include "leds.h"
#include "comm.h"

   
int main()
{
    leds_init();
    button_init();
    comm_init();
    if(tcp_server_init() == -1)
    {
        exit(0);
    }
    while(1)
    {
        tcp_server_send();
    }
}