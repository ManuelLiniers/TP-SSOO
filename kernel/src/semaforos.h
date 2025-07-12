#ifndef SEMAFOROS_H_
#define SEMAFOROS_H_

#include <semaphore.h>
#include "utils/commons.h"

// Semaforos y Mutex
extern pthread_mutex_t mutex_queue_new;
extern pthread_mutex_t mutex_queue_ready;
extern pthread_mutex_t mutex_queue_block;
extern pthread_mutex_t mutex_queue_exit;
extern pthread_mutex_t mutex_queue_susp_ready;
extern pthread_mutex_t mutex_procesos_ejecutando;

extern sem_t bloqueante_sem;
extern sem_t cpu_libre;

extern sem_t nuevo_proceso;
extern sem_t proceso_ready;
extern sem_t espacio_memoria;

void iniciar_semaforos();
void signal_sem(sem_t *sem);
void wait_sem(sem_t *sem);
void signal_mutex(pthread_mutex_t *mutex);
void wait_mutex(pthread_mutex_t *mutex);
void destruir_semaforos();

#endif