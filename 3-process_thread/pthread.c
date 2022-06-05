#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void *t_function(void *data)
{
    pid_t pid;      // process id
    pthread_t tid;  // thread id

    pid = getpid();
    tid = pthread_self();

    char *thread_name = (char *)data;
    int i = 0;

    // 3번 
    while (i < 3){
        printf("[%s] pid:%u, tid:%x --- %d\n", thread_name, (unsigned int)pid, (unsigned int)tid, i);
        i++;
        sleep(1);
    }
}

int main()
{
    pthread_t p_thread[2];
    int thr_id;
    int status;
    char p1[] = "thread_1";
    char p2[] = "thread_2";
    char pM[] = "thread_m";

    // 쓰레드 만들어서 거기서 t_function 실행 해달라고 하는거
    thr_id = pthread_create(&p_thread[0], NULL, t_function, (void *)p1);
    if (thr_id < 0)
    {
        perror("thread create error : ");
        exit(0);
    }

    thr_id = pthread_create(&p_thread[1], NULL, t_function, (void *)p2);
    if (thr_id < 0)
    {
        perror("thread create error : ");
        exit(0);
    }

    // 지금 여기서 t_function 하라고 => 비동기
    // 순서는 랜덤임
    t_function((void *)pM);
    pthread_join(p_thread[0], (void **)&status);
    pthread_join(p_thread[1], (void **)&status);

    return 0;
}