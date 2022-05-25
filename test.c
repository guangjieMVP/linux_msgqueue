#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "lin_msgqueue.h"

lin_mq_t *myqueue = NULL;

void *recv_thread_entry( void  *arg)
{
    lin_mq_blk_t *msg;
    char buf[1024] = {0};

    while (1)
    {
        if (lin_mq_get_msg(myqueue, buf, sizeof(buf)) == LIN_MSGQ_EOK)
        {
            int len = strlen(buf);
            for (int i = 0; i < len; i++)
            {
                printf("%c", buf[i]);
            }
            printf("\r\n");
            while (1)
            {
                if (lin_mq_get_msg_timeout(myqueue, buf, sizeof(buf), 2000) == LIN_MSGQ_EOK)
                {
                    int len = strlen(buf);
                    for (int i = 0; i < len; i++)
                    {
                        printf("%c", buf[i]);
                    }
                }
                else
                {
                    printf("wait timeout\r\n");
                    break;
                }
            }
            break;
        }
    }
    return  NULL;
}

int test_func(int **p)
{
    int *m = (int *)malloc(sizeof(int));
    printf("%d %p\r\n", __LINE__, m);
    *p = m;
    return 0;
}

int main(int argc, char **argv)
{
    int ret;
    pthread_t recv_thread;

    myqueue = lin_mq_create();  
    if (myqueue == NULL)
        return -1;

    ret = pthread_create(&recv_thread, NULL, recv_thread_entry, NULL);
     if  (ret != 0)
        printf ( "can't create thread: %s\n" , strerror (ret));
    
    char buf[1024] = "https://blog.csdn.net/xiaoyuanwuhui/article/details/110538555";
    if (lin_mq_send_msg(myqueue, buf, strlen(buf)) != 0)
    {
        printf("send err\r\n");
    }

    sleep(2);

    pthread_join(recv_thread, NULL);

    int *ptr;

    test_func(&ptr);
    printf("%d %p\r\n", __LINE__, ptr);
    

    return 0;
}
