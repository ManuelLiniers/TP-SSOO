#ifndef PCB_H_
#define PCB_H_

#include <commons/collections/list.h>
#include "utils/commons.h"
#include "semaforos.h"

extern int pid_incremental;
extern int id_io_incremental;

extern t_list* lista_dispositivos_io;
void iniciar_dispositivos_io();
extern t_list* lista_cpus;
void iniciar_cpus();

// Estados posibles de un proceso
typedef enum { NEW, READY, EXEC, BLOCKED, EXIT } t_estado;

int id_estado(t_estado estado);

extern t_queue* queue_new;
extern t_list* queue_new_PMCP;
extern t_queue* queue_ready;
extern t_list* queue_block;
extern t_queue* queue_exit;

typedef struct{
    t_estado estado;
    int tiempo_inicio;
    int tiempo_fin;
 } t_metricas_estado_tiempo;

// Control Block de un proceso
typedef struct {
    int pid;                    // Identificador único
    int program_counter;        // Contador de instrucción
    char* instrucciones;      // Lista de instrucciones (strings o structs)
    t_estado estado;            // Estado actual del proceso
    int tamanio_proceso; 
    int metricas_estado[5];         // Lista de veces que estuvo en cada estado
    t_list* metricas_tiempo;  // Lista de tiempo que estuvo en cada estado
    int registros[4];
    // Más campos opcionales: tamaño de memoria, registros, métricas, etc.
} t_pcb;

extern t_list* lista_procesos_ejecutando;


/**
 * @brief Crea un PCB inicializado
 * @param pid Identificador del proceso
 * @param instrucciones Lista de instrucciones (creada externamente)
 * @return puntero a t_pcb o NULL en caso de error
 */
t_pcb* pcb_create();

/**
 * @brief Destruye un PCB liberando instrucciones y la propia estructura
 * @param pcb_void puntero a t_pcb (pasado como void* para destruir en queues)
 */
void pcb_destroy(void* pcb_void);

void crear_proceso(char* instrucciones, char* tamanio_proceso);

typedef struct{
    char cpu_id[20];
    int socket_dispatch;
    int socket_interrupt;
    bool esta_libre;
} t_cpu;

typedef struct{
    char nombre[20];
    int id;
    int socket;
} t_dispositivo_io;

typedef struct{
    t_pcb *pcb;
    int tiempo;
} tiempo_en_io;

#endif // PCB_H_
