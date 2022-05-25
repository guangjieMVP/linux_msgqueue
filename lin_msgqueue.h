#ifndef __LIN_MSGQUEUE_H__
#define __LIN_MSGQUEUE_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define LIN_MSG_MAX_LEN    4096      /* max 4K */

/* msgqueue return state */
#define LIN_MSGQ_EOK     0          /**< There is no error */                
#define LIN_MSGQ_ERROR   1           /**< A generic error happens */
#define LIN_MSGQ_EINVAL  2           /**< Invalid argument */
#define LIN_MSGQ_ENOMEM  3            /**< No memory */
#define LIN_MSGQ_TIMEOUT 4            /**< Timeout> */

struct lin_msgq_block;
typedef struct lin_msgq_block lin_mq_blk_t;

/* message block */
struct lin_msgq_block {
    lin_mq_blk_t *next;  
	int len;
    char *data;
};
  
struct lin_msgqueue;
typedef struct lin_msgqueue lin_mq_t;

struct lin_msgqueue
{
    lin_mq_blk_t *first;
    lin_mq_blk_t *last;
    int     num;
 
    pthread_mutex_t mutex;
    pthread_cond_t  not_empty;
};
 
 
lin_mq_t *lin_mq_create(void);
int lin_mq_destroy(lin_mq_t *mq);
int lin_mq_send_msg(lin_mq_t *mq, char *msg, int msg_len);
int lin_mq_get_msg(lin_mq_t *mq, char *buffer, int size);
int lin_mq_get_msg_timeout(lin_mq_t *mq, char *buffer, int size, int timeout);
int lin_mq_get_msg_num(lin_mq_t *mq);
int lin_mq_clear(lin_mq_t *mq);

#endif  /* __LIN_MSGQUEUE_H__ */