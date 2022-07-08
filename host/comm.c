#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <mqueue.h>
#include <stdbool.h>
#include <semaphore.h>
#include "comm.h"


#define LED_ON_CMD          "turn_on"
#define LED_OFF_CMD         "turn_off"

#define CMD_MQ_NAME         "/led_command_mq"
#define STAT_MQ_NAME        "/led_status_mq"

#define LED_SHM_NAME        "/led_shm"

#define LED_SEM_NAME        "/led_sem"

typedef struct 
{
    char command[20];
    char status[50];
}led_info_t;

static mqd_t cmd_mq_fd;
static mqd_t stat_mq_fd;

/* Note: only one led data structure is used in order to demonstrate shared memory and semaphores
   more efficiently */

static int led_shm_fd;
static char * led_shm_addr;

static sem_t * led_semdes = SEM_FAILED;

/**
 * @brief Initialize comm semaphores 
 * 
 * @return int 0 on success, -1 if failed
 */
static int comm_sem_init(void)
{
    led_semdes = sem_open(LED_SEM_NAME, O_CREAT | O_EXCL | O_RDWR, S_IRWXU, 1);
    if (led_semdes == SEM_FAILED)
    {
        perror("ERROR: sem_open() failed\n");
        if(!strcmp("File exists", strerror(errno)))
        {
            // File exists so try regular open
            led_semdes = sem_open(LED_SEM_NAME, O_EXCL | O_RDWR, S_IRWXU, 1);
            if (led_semdes == SEM_FAILED)
            {
                perror("ERROR: sem_open() failed again\n");
                return -1;
            }
        }
        else // File exists
        {
            return -1;
        }
    }
    printf("Semaphores successfully created\n");
    return 0;
}

/**
 * @brief Initialize the communication queues
 * 
 * @return int -1 on error, 0 on success
 */
static int comm_mq_init(void)
{
    struct mq_attr cmd_attr;
    struct mq_attr stat_attr;
    cmd_mq_fd = -1;
    stat_mq_fd = -1;

    // Set the command message queue
    cmd_attr.mq_maxmsg = 1;
    cmd_attr.mq_msgsize = sizeof(bool);
    cmd_mq_fd = mq_open(CMD_MQ_NAME, O_CREAT | O_RDWR, S_IRWXU, &cmd_attr);
    if(cmd_mq_fd < 0)
    {
        perror("cmd_mq_fd mq_open failed\n");
        goto error;
    }
    
    // Set the status message queue
    stat_attr.mq_maxmsg = 1;
    stat_attr.mq_msgsize = sizeof(bool);
    stat_mq_fd = mq_open(STAT_MQ_NAME, O_CREAT | O_RDWR, S_IRWXU, &stat_attr); 
    if(stat_mq_fd < 0)
    {
        perror("stat_mq_fd mq_open failed\n");
        goto error;
    }

    printf("message queues successfully created\n");
    return 0;

error:
    printf("comm_mq_init failed\n");
    return -1;
}

/**
 * @brief Initialize the shared memory
 * 
 * @return int -1 on error, 0 on success
 */
static int comm_shm_init(void)
{
    // Set the command shared memory
    led_shm_fd = shm_open(LED_SHM_NAME, O_CREAT | O_RDWR, S_IRWXU);
    if(led_shm_fd < 0)
    {
        goto error;
    }
    if(ftruncate(led_shm_fd, sizeof(sizeof(led_info_t))) != 0)
    {
        perror("ERROR: ftruncate() failed\n");
        goto error;
    }
    led_shm_addr = (char *) mmap (NULL, sizeof(led_info_t), PROT_READ | PROT_WRITE, MAP_SHARED, led_shm_fd, 0);
    if(led_shm_addr == MAP_FAILED)
    {
        perror("ERROR: mmap() failed\n");
        goto error;
    }
    printf("mmap() returned %p\n", led_shm_addr); 

    printf("shared memory successfully created\n");
    return 0;

error:
    printf("comm_shm_init failed\n");
    return -1;
}

/**
 * @brief Deinitialized comm semaphores 
 * 
 */
static void comm_sem_deinit(void)
{
    if(sem_close(led_semdes) != 0)
    {
        perror("sem_close() failed\n");
    }
}

/**
 * @brief Deinitialize the communication queues
 * 
 */
static void comm_mq_deinit(void)
{
    if(mq_close(cmd_mq_fd) != 0)
    {
        perror("cmd mq closed failed\n");
    }
    if(mq_close(stat_mq_fd) != 0)
    {
        perror("stat mq closed failed\n");
    }
}

/**
 * @brief Deinitialized the shared memory
 */
static void comm_shm_deinit(void)
{
    munmap(led_shm_addr, sizeof(led_info_t));
    close(led_shm_fd);
    shm_unlink(LED_SHM_NAME);
}

int comm_init(void)
{
    if(comm_mq_init() || comm_shm_init() || comm_sem_init())
    {
        printf("Error: comm_init failed\n");
        return -1;
    }
    return 0;
}

/**
 * @brief Best try to deinit initialized
 * shm, mq and semaphore
 */
void comm_deinit(void)
{
    comm_mq_deinit();
    comm_shm_deinit();
    comm_sem_deinit();
}

/**
 * @brief Send command writes to the shared memory and notifies via the
 * mq that a message is waiting for sending
 * 
 * @param command The command wished to be sent
 */
void comm_send_command(char * command)
{
    bool new_cmd_notify = true;
    led_info_t temp;
    if(command == NULL)
    {
        perror("Error: command param is null\n");
        return;
    }
    // Check if the command is turn_on or turn_off
    if(!strcmp(command, LED_OFF_CMD) || !strcmp(command, LED_ON_CMD)) // The led should be turned on
    {
        // First take the semaphore for read/write into the shared memory
        if(sem_wait(led_semdes) != 0)
        {
            perror("sem_wait failed\n");
            return;
        }
        memcpy(&temp, led_shm_addr, sizeof(led_info_t));
        strcpy(temp.command, command);
        memcpy(led_shm_addr, &temp, sizeof(led_info_t));
        // Now release the semaphore
        if(sem_post(led_semdes) != 0)
        {
            while(sem_post(led_semdes) != 0)
            {
                perror("sem_post failed\n");
                usleep(500000);
            }
        }
        if(mq_send(cmd_mq_fd, (char *)&new_cmd_notify, sizeof(bool), 10) != 0)
        {
            perror("mq_send failed\n");
            return;
        }
    }
    else // Not one of the specified commands
    {
        printf("ERROR: Wrong command\n");
    }
    printf("comm_send_command : %s \n", command);
}

/**
 * @brief Wait for command to be received in the shm,
 * will return only if error occured or the command has arrived
 * 
 * @param command The command extracted from shm
 */
int comm_receive_command(char * command)
{
    bool rcv_flag = false;
    led_info_t temp;
    mq_receive(cmd_mq_fd, (char *)&rcv_flag, sizeof(bool), NULL);
    if(rcv_flag == false)
    {
        printf("ERROR: False is reveice in cmd msg queue\n");
        return -1;
    }
    // First take the semaphore for read/write into the shared memory
    if(sem_wait(led_semdes) != 0)
    {
        perror("sem_wait failed\n");
        return -1;
    }
    memcpy(&temp, led_shm_addr, sizeof(led_info_t));
    // Now release the semaphore
    if(sem_post(led_semdes) != 0)
    {
        while(sem_post(led_semdes) != 0)
        {
            perror("sem_post failed\n");
            usleep(500000);
        }
    }
    strcpy(command, temp.command);
    printf("comm_receive_command : %s \n", command);
    return 0;
}

/**
 * @brief Re-send latest command, bassically just send another notification
 */
void comm_resend_command(void)
{
    bool new_cmd_notify = true;
    if(mq_send(cmd_mq_fd, (char *)&new_cmd_notify, sizeof(bool), 10) != 0)
    {
        perror("mq_send failed\n");
        return;
    }
}

/**
 * @brief Send status writes to the shared memory and notifies 
 * via the mq that a message is waiting for sending
 * 
 * @param status The status wished to be sent
 */
void comm_send_status(char * status)
{
    bool new_stat_notify = true;
    led_info_t temp;
    if(status == NULL)
    {
        printf("Error: status param is null\n");
        return;
    }
    // First take the semaphore for read/write into the shared memory
    if(sem_wait(led_semdes) != 0)
    {
        printf("sem_wait failed\n");
        return;
    }
    memcpy(&temp, led_shm_addr, sizeof(led_info_t));
    strcpy(temp.status, status);
    memcpy(led_shm_addr, &temp, sizeof(led_info_t));
    // Now release the semaphore
    if(sem_post(led_semdes) != 0)
    {
        while(sem_post(led_semdes) != 0)
        {
            printf("sem_post failed\n");
            usleep(500000);
        }
    }
    if(mq_send(stat_mq_fd, (char *)&new_stat_notify, sizeof(bool), 0) != 0)
    {
        printf("mq_send failed\n");
        return;
    }
    printf("comm_send_status : %s \n", status);
}

/**
 * @brief Wait for status to be received in the shm,
 * will return only if error occured or the status has arrived
 * 
 * @param status The status extracted from shm
 */
int comm_receive_status(char * status)
{
    bool rcv_flag = false;
    led_info_t temp;
    mq_receive(stat_mq_fd, (char *)&rcv_flag, sizeof(bool), NULL);
    if(rcv_flag == false)
    {
        printf("ERROR: False is reveice in stat msg queue\n");
        return -1;
    }
    // First take the semaphore for read/write into the shared memory
    if(sem_wait(led_semdes) != 0)
    {
        printf("sem_wait failed\n");
        return -1;
    }
    memcpy(&temp, led_shm_addr, sizeof(led_info_t));
    // Now release the semaphore
    if(sem_post(led_semdes) != 0)
    {
        while(sem_post(led_semdes) != 0)
        {
            printf("sem_post failed\n");
            usleep(500000);
        }
    }
    strcpy(status, temp.status);
    printf("comm_receive_status : %s\n", status);
    return 0;
}