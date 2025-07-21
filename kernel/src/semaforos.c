#include "semaforos.h"

// Mutexes
pthread_mutex_t mutex_queue_susp_ready;
pthread_mutex_t mutex_queue_new;
pthread_mutex_t mutex_queue_ready;
pthread_mutex_t mutex_queue_block;
pthread_mutex_t mutex_queue_exit;
pthread_mutex_t mutex_lista_cpus;
pthread_mutex_t mutex_procesos_ejecutando;
pthread_mutex_t mutex_memoria_swap;
pthread_mutex_t mutex_lista_dispositivos_io;

pthread_mutex_t mutex_creacion_hilos;
pthread_mutex_t mutex_pcb;
pthread_mutex_t mutex_diccionario_io;
pthread_mutex_t mutex_pid_inc;

pthread_mutex_t desalojando;
pthread_mutex_t mutex_susp_o_memoria;
pthread_mutex_t mutex_sem_espacio_memoria;
pthread_mutex_t comprobar_memoria;
pthread_mutex_t desalojo_revisado;
pthread_mutex_t check_desalojo;

pthread_mutex_t mutex_cpu;


// Semáforos
sem_t nuevo_proceso;
sem_t espacio_memoria;
sem_t proceso_ready;
//sem_t check_desalojo;
sem_t nuevo_proceso_suspendido_ready;
sem_t proceso_suspendido_ready;

sem_t cpu_libre;
sem_t bloqueante_sem;
sem_t dispositivo_libre;
sem_t planificacion_principal;
sem_t ver_desalojo;

sem_t planificar;
sem_t largo_plazo;

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
    pthread_mutex_init(&mutex_cpu, NULL);
    pthread_mutex_init(&mutex_lista_cpus, NULL);
    pthread_mutex_init(&mutex_memoria_swap, NULL);
    pthread_mutex_init(&mutex_lista_dispositivos_io, NULL);
    pthread_mutex_init(&mutex_creacion_hilos, NULL);
    pthread_mutex_init(&mutex_pcb, NULL);
    pthread_mutex_init(&mutex_pid_inc, NULL);
    pthread_mutex_init(&mutex_diccionario_io, NULL);
    pthread_mutex_init(&desalojando, NULL);
    pthread_mutex_init(&mutex_sem_espacio_memoria, NULL);
    pthread_mutex_init(&mutex_susp_o_memoria, NULL);
    pthread_mutex_init(&comprobar_memoria, NULL);
    pthread_mutex_init(&desalojo_revisado, NULL);
    pthread_mutex_init(&check_desalojo, NULL);
    
    sem_init(&nuevo_proceso, 0, 0);
    sem_init(&proceso_ready, 0, 0);
    sem_init(&espacio_memoria, 0, 0);
    sem_init(&bloqueante_sem, 0, 0);
    sem_init(&cpu_libre, 0, 0);
    sem_init(&dispositivo_libre, 0, 0);
    sem_init(&nuevo_proceso_suspendido_ready, 0, 0);
    //sem_init(&check_desalojo, 0, 0);
    sem_init(&planificacion_principal, 0, 0);
    sem_init(&ver_desalojo, 0, 0);
    sem_init(&proceso_suspendido_ready, 0, 0);
    sem_init(&planificar, 0, 0);
    sem_init(&largo_plazo, 0, 0);
};

void destruir_semaforos() {
    pthread_mutex_destroy(&mutex_queue_new);
    pthread_mutex_destroy(&mutex_queue_ready);
    pthread_mutex_destroy(&mutex_queue_block);
    pthread_mutex_destroy(&mutex_queue_exit);
    pthread_mutex_destroy(&mutex_queue_susp_ready);
    pthread_mutex_destroy(&mutex_procesos_ejecutando);
    pthread_mutex_destroy(&mutex_lista_dispositivos_io);
    pthread_mutex_destroy(&mutex_cpu);
    pthread_mutex_destroy(&mutex_lista_cpus);
    pthread_mutex_destroy(&mutex_memoria_swap);
    pthread_mutex_destroy(&mutex_creacion_hilos);
    pthread_mutex_destroy(&mutex_pcb);
    pthread_mutex_destroy(&mutex_diccionario_io);
    pthread_mutex_destroy(&desalojando);
    pthread_mutex_destroy(&mutex_sem_espacio_memoria);
    pthread_mutex_destroy(&mutex_susp_o_memoria);
    pthread_mutex_destroy(&comprobar_memoria);
    pthread_mutex_destroy(&desalojo_revisado);
    pthread_mutex_destroy(&check_desalojo);

    sem_destroy(&nuevo_proceso);
    sem_destroy(&proceso_ready);
    sem_destroy(&espacio_memoria);
    sem_destroy(&ver_desalojo);
    sem_destroy(&planificar);
};
