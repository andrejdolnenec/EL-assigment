/**
 * @file comm.h
 * @author your name (you@domain.com)
 * @brief File containing functions for inter-thread communication
 * @version 0.1
 * @date 2022-05-02
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

/**
 * @brief Initialize the communication queues and shared memory
 * 
 * @return int 0 on success, -1 on error
 */
int comm_init(void);

/**
 * @brief Send command writes to the shared memmory and notifies 
 * via the mq that a message is waiting for sending
 * 
 * @param command The command wished to be sent
 */
void comm_send_command(char * command);

/**
 * @brief Wait for command to be received in the shm,
 * will return only if error occured or the command has arrived
 * 
 * @param command The command extracted from shm
 */
int comm_receive_command(char * command);

/**
 * @brief Re-send latest command, bassically just send another notification
 */
void comm_resend_command(void);

/**
 * @brief Send status writes to the shared memory and notifies 
 * via the mq that a message is waiting for sending
 * 
 * @param status The status wished to be sent
 */
void comm_send_status(char * status);

/**
 * @brief Wait for status to be received in the shm,
 * will return only if error occured or the status has arrived
 * 
 * @param status The status extracted from shm
 */
int comm_receive_status(char * status);

/**
 * @brief Best try to deinit initialized
 * shm, mq and semaphore
 */
void comm_deinit(void);