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
extern pthread_mutex_t mutex_cpu;
extern pthread_mutex_t mutex_lista_cpus;
extern pthread_mutex_t mutex_memoria_swap;
extern pthread_mutex_t mutex_lista_dispositivos_io;
extern pthread_mutex_t mutex_pcb;
extern pthread_mutex_t mutex_diccionario_io;
extern pthread_mutex_t desalojando;
extern pthread_mutex_t mutex_sem_espacio_memoria;
extern pthread_mutex_t mutex_susp_o_memoria;
extern pthread_mutex_t comprobar_memoria;

extern pthread_mutex_t mutex_creacion_hilos;
extern pthread_mutex_t mutex_pid_inc;



extern sem_t bloqueante_sem;
extern sem_t cpu_libre;
extern sem_t ver_desalojo;

extern sem_t nuevo_proceso;
extern sem_t proceso_ready;
extern sem_t check_desalojo;
extern sem_t desalojo_revisado;
extern sem_t planificacion_principal;
extern sem_t espacio_memoria;
extern sem_t dispositivo_libre;
extern sem_t nuevo_proceso_suspendido_ready;
extern sem_t proceso_suspendido_ready;

extern sem_t planificar;
extern sem_t largo_plazo;

void iniciar_semaforos();
void signal_sem(sem_t *sem);
void wait_sem(sem_t *sem);
void signal_mutex(pthread_mutex_t *mutex);
void wait_mutex(pthread_mutex_t *mutex);
void destruir_semaforos();

#endif