#include "semaforos.h"


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


void iniciar_semaforos(){
    pthread_mutex_init(&mutex_queue_new, NULL);
    pthread_mutex_init(&mutex_queue_ready, NULL);
    pthread_mutex_init(&mutex_queue_block, NULL);
    pthread_mutex_init(&mutex_queue_exit, NULL);

    sem_init(&nuevo_proceso, 0, 0);
    sem_init(&proceso_ready, 0, 0);
    sem_init(&proceso_terminado, 0, 0);
};

void destruir_semaforos() {
    pthread_mutex_destroy(&mutex_queue_new);
    pthread_mutex_destroy(&mutex_queue_ready);
    pthread_mutex_destroy(&mutex_queue_block);
    pthread_mutex_destroy(&mutex_queue_exit);

    sem_destroy(&nuevo_proceso);
    sem_destroy(&proceso_ready);
    sem_destroy(&proceso_terminado);
};
