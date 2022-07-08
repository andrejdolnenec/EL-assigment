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
 * @brief Send led status to msg queue
 * 
 * @param status The status to be written in shm
 */
void comm_send_led_status(char * status);

/**
 * @brief Receive led status from msg queue
 * 
 * @param status The status obtained from shm
 * @return int 0 on success, -1 on error
 */
int comm_receive_led_status(char * status);

/**
 * @brief Best try to deinit initialized
 * shm, mq and semaphore
 */
void comm_deinit(void);