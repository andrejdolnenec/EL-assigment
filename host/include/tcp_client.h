/**
 * @file tcp_client.h
 * @author your name (you@domain.com)
 * @brief File containing tcp client functions
 * @version 0.1
 * @date 2022-05-02
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

/**
 * @brief Initialize the tcp client, connect to server
 * 
 * @param ip_addr the ip address to which to connect
 * @return int 0 on success, -1 on failure
 */
int tcp_client_init(char * ip_addr);

/**
 * @brief Close the socket oppened from the connection
 */
void tcp_client_deinit(void);