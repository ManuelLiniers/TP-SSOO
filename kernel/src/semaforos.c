#include<semaforos.h>


// wait y signal para semaforos
void wait_mutex(pthread_mutex_t *mutex){
    pthread_mutex_lock(mutex);
}
void signal_mutex(pthread_mutex_t *mutex){
    pthread_mutex_unlock(mutex);
}
void wait_sem(sem_t *sem)
{
    sem_wait(sem);
}
void signal_sem(sem_t *sem)
{
    sem_post(sem);
}


void iniciar_mutex(){
    pthread_mutex_init(&mutex_queue_new, NULL);
    pthread_mutex_init(&mutex_queue_ready, NULL);
    pthread_mutex_init(&mutex_queue_block, NULL);
    pthread_mutex_init(&mutex_queue_exit, NULL);
};