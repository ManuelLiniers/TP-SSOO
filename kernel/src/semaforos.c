#include "semaforos.h"

// Mutexes
pthread_mutex_t mutex_queue_new;
pthread_mutex_t mutex_queue_ready;
pthread_mutex_t mutex_queue_block;
pthread_mutex_t mutex_queue_exit;
pthread_mutex_t mutex_queue_susp_ready;
pthread_mutex_t mutex_procesos_ejecutando;

// Semáforos
sem_t nuevo_proceso;
sem_t espacio_memoria;
sem_t proceso_ready;

sem_t cpu_libre;
sem_t bloqueante_sem;


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
    pthread_mutex_init(&mutex_queue_susp_ready, NULL);
    pthread_mutex_init(&mutex_procesos_ejecutando, NULL);
    
    sem_init(&nuevo_proceso, 0, 0);
    sem_init(&proceso_ready, 0, 0);
    sem_init(&espacio_memoria, 0, 0);
    sem_init(&bloqueante_sem, 0, 0);
    sem_init(&cpu_libre, 0, 0);
};

void destruir_semaforos() {
    pthread_mutex_destroy(&mutex_queue_new);
    pthread_mutex_destroy(&mutex_queue_ready);
    pthread_mutex_destroy(&mutex_queue_block);
    pthread_mutex_destroy(&mutex_queue_exit);
    pthread_mutex_destroy(&mutex_queue_susp_ready);
    pthread_mutex_destroy(&mutex_procesos_ejecutando);

    sem_destroy(&nuevo_proceso);
    sem_destroy(&proceso_ready);
    sem_destroy(&espacio_memoria);
};
