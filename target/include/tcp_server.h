
#pragma once

/**
 * @brief Initialize the tcp server, wait for connection
 * 
 * @return int connection fd on success, -1 on failure
 */
int tcp_server_init(void);

/**
 * @brief Wait for status to arrive and send it via tcp
 * @note This is a blocking function
 */
void tcp_server_send(void);

/**
 * @brief Close the socket oppened from the connection
 */
void tcp_server_deinit(void);