#include "lin_msgqueue.h"

#define SLOG_USE_CUSTOM   1

#if SLOG_USE_CUSTOM == 1
#define _PRINT   printf
#else
#define _PRINT 
#endif
 
#define __FILENAME__ (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1) : __FILE__)

#define SLOG_PRINT(X, args...) _PRINT("[%s]-%d:" X, __FILENAME__, __LINE__, ##args)

static unsigned long long timeout_ns = 0;

static void lin_mq_free_msg_blk(lin_mq_blk_t *elm)
{
    if (elm != NULL)
    {
        if (elm->data != NULL)
        {
            free(elm->data);
            elm->data = NULL;
        }
        free(elm);
        elm = NULL;
    }
}

static void put_msg_to_queue(lin_mq_t *mq, lin_mq_blk_t *elm)
{
    if (NULL == mq || NULL == elm)
    {
        SLOG_PRINT("mq or elm is NULL");
        return;
    }

    if (NULL != elm->next)
    {
        elm->next = NULL;
    }

    pthread_mutex_lock(&mq->mutex);
    /* message queue have no message block */
    if (mq->first == mq->last && 0 == mq->num)
    {
        mq->first = elm;
        mq->last = elm;
        mq->num++;
        /* There is a new message block, Wake up a blocked thread */
        pthread_cond_signal(&mq->not_empty);
    }
    else
    {
        /* Insert the new message block at the end of the queue */
        mq->last->next = elm;
        mq->last = elm;
        mq->num++;
    }

    pthread_mutex_unlock(&mq->mutex);
}

static int lin_mq_get_msg_blk(lin_mq_t *mq, lin_mq_blk_t **blk)
{
    if (NULL == mq)
    {
        SLOG_PRINT("mq or elm is NULL");
        return -LIN_MSGQ_EINVAL;
    }

    pthread_mutex_lock(&mq->mutex);
    /* message queue have no message block */
    while (0 == mq->num)
    {
        /* blocking threads and wait message block */
        pthread_cond_wait(&mq->not_empty, &mq->mutex);
    }
    /* get new message block from message queue */
    *blk = (mq->first);
    if (1 == mq->num)
    {
        mq->first = mq->last = NULL;
        mq->num = 0;
    }
    else
    {
        mq->first = mq->first->next;
        mq->num--;
    }

    pthread_mutex_unlock(&mq->mutex);

    return LIN_MSGQ_EOK;
}

int lin_mq_get_msg(lin_mq_t *mq, char *buffer, int size)
{
    int ret;
    lin_mq_blk_t *blk;
    ret = lin_mq_get_msg_blk(mq, &blk);  
    if (ret != LIN_MSGQ_EOK) return ret;

    if (blk->data != NULL)
    {
        size = size > blk->len ? blk->len : size;
        memmove(buffer, blk->data, size);
        lin_mq_free_msg_blk(blk);
    }
    return LIN_MSGQ_EOK;
}

static int lin_mq_get_msg_blk_timeout(lin_mq_t *mq, lin_mq_blk_t **blk, int timeout)
{
    struct timespec timenow;

    if (NULL == mq)
    {
        SLOG_PRINT("mq or elm is NULL");
        return -LIN_MSGQ_EINVAL;
    }

    pthread_mutex_lock(&mq->mutex);
    /* There is no message block */
    if (0 == mq->num)
    {
        clock_gettime(CLOCK_MONOTONIC, &timenow);
        timenow.tv_sec = timenow.tv_sec + timeout / 1000; /* add sec */
        timeout %= 1000;                                  /* get msec */

        timeout_ns = timenow.tv_nsec + timeout * 1000 * 1000;  /* get nsec */
        /* If more than 1 s */
        if (timeout_ns >= 1000 * 1000 * 1000) 
        {
            timenow.tv_sec++;
            timenow.tv_nsec = timeout_ns - 1000 * 1000 * 1000;
        }
        else
            timenow.tv_nsec = timeout_ns;

        /* blocking threads and wait timeout */
        pthread_cond_timedwait(&mq->not_empty, &mq->mutex, &timenow);
    }

    if (mq->num <= 0)
    {   
        pthread_mutex_unlock(&mq->mutex);
        return -LIN_MSGQ_TIMEOUT;
    }

    /* get the message block from the message queue head */
    *blk = mq->first;
    if (1 == mq->num)
    {
        mq->first = mq->last = NULL;
        mq->num = 0;
    }
    else
    {
        mq->first = mq->first->next;
        mq->num--;
    }

    pthread_mutex_unlock(&mq->mutex);

    return LIN_MSGQ_EOK;
}

int lin_mq_get_msg_timeout(lin_mq_t *mq, char *buffer, int size, int timeout)
{
    int ret;
    lin_mq_blk_t *blk = NULL;

    ret = lin_mq_get_msg_blk_timeout(mq, &blk, timeout);  
    if (ret != LIN_MSGQ_EOK) return ret;

    if (blk->data == NULL)
    {
        SLOG_PRINT("message block data is NULL\r\n");
        return -LIN_MSGQ_ENOMEM;
    }

    size = size > blk->len ? blk->len : size;
    memmove(buffer, blk->data, size);
    lin_mq_free_msg_blk(blk);
    return LIN_MSGQ_EOK;
}

int lin_mq_clear(lin_mq_t *mq)
{
    lin_mq_blk_t *blk = NULL;
    lin_mq_blk_t *blk_tmp = NULL;

    if (NULL == mq)
    {
        SLOG_PRINT("mq is NULL");
        return -LIN_MSGQ_EINVAL;
    }

    if (mq->num <= 0)
    {
        SLOG_PRINT("mq->num is 0\r\n");
        return -LIN_MSGQ_ERROR;
    }

    pthread_mutex_lock(&mq->mutex);
    /* Clears all message nodes in the buffer before the current message node */
    blk = mq->first;
    while (blk != NULL)
    {
        if (blk == mq->last)
        {
            mq->first = mq->last;
            if (mq->num != 1)
            {
                mq->num = 1;
            }
            break;
        }

        blk_tmp = blk->next;
        lin_mq_free_msg_blk(blk);
        mq->num--;
        blk = blk_tmp;
        mq->first = blk;
    }

    pthread_mutex_unlock(&mq->mutex);

    return LIN_MSGQ_EOK;
}

int lin_mq_send_msg(lin_mq_t *mq, char *msg, int msg_len)
{
    lin_mq_blk_t *blk = NULL;

    if (msg == NULL) 
    {
        SLOG_PRINT("msg is NULL!!!");
        return -LIN_MSGQ_EINVAL;
    }

    blk = (lin_mq_blk_t *)malloc(sizeof(lin_mq_blk_t));
    if (NULL == blk)
    {
        SLOG_PRINT("new msg element failed!!");
        return -LIN_MSGQ_ENOMEM;
    }
    /* Limit maximum length */
    if (msg_len > LIN_MSG_MAX_LEN) 
    {
        msg_len = LIN_MSG_MAX_LEN;
        SLOG_PRINT("msg %d!", LIN_MSG_MAX_LEN);
    }

    memset(blk, 0, sizeof(lin_mq_blk_t));
    blk->len = msg_len;
    /* Allocate memory based on the sent size */
    blk->data = (char *)malloc(msg_len); 
    if (blk->data == NULL)
    {
        SLOG_PRINT("new element->data failed!!");
        lin_mq_free_msg_blk(blk);
        return -LIN_MSGQ_ENOMEM;
    }

    memmove(blk->data, msg, msg_len);

    blk->next = NULL;

    put_msg_to_queue(mq, blk);

    return LIN_MSGQ_EOK;
}

int lin_mq_destroy(lin_mq_t *mq)
{
    lin_mq_blk_t *blk = NULL;

    if (NULL == mq)
    {
        return (-LIN_MSGQ_EINVAL);
    }

    if (mq->first != mq->last && mq->num > 0)
    {
        blk = lin_mq_clear(mq);
    }
    else
    {
        blk = mq->last;
    }

    if (NULL != blk)
    {
        lin_mq_free_msg_blk(blk);
        mq->first = mq->last = NULL;
        mq->num = 0;
    }

    pthread_mutex_destroy(&mq->mutex);
    pthread_cond_destroy(&mq->not_empty);
    free(mq);

    mq = NULL;
    return (LIN_MSGQ_EOK);
}

int lin_mq_get_msg_num(lin_mq_t *mq)
{
    if (NULL == mq)
        return (-LIN_MSGQ_EINVAL);

    return mq->num;
}


lin_mq_t *lin_mq_create(void)
{
    lin_mq_t *mq = NULL;
    pthread_condattr_t cattr;

    mq = (lin_mq_t *)malloc(sizeof(lin_mq_t));
    if (NULL == mq)
    {
        SLOG_PRINT("init msg buffer failed!!");
        return NULL;
    }

    memset(mq, 0, sizeof(lin_mq_t));
    mq->first = NULL;
    mq->last = NULL;
    mq->num = 0;

    pthread_mutex_init(&(mq->mutex), NULL);
#if 1
    pthread_condattr_init(&cattr);
    pthread_condattr_setclock(&cattr, CLOCK_MONOTONIC);
    pthread_cond_init(&(mq->not_empty), &cattr);
#else
    pthread_cond_init(&(mq->not_empty), NULL);
#endif

    return mq;
}



