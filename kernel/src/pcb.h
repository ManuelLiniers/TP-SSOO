#ifndef PCB_H_
#define PCB_H_

#include <commons/collections/list.h>
#include "utils/commons.h"
#include "scheduler.h"
#include "semaforos.h"

int pid_incremental = 0;

// Estados posibles de un proceso
typedef enum { NEW, READY, EXEC, BLOCKED, EXIT } t_estado;

t_estado posibles_estados[5] = {NEW, READY, EXEC, BLOCKED, EXIT};

struct t_metricas_cant{
    t_estado estado;
    int cant;
    struct t_metricas_cant* sig;
}; 
typedef struct t_metricas_cant t_metricas_cant;

struct t_metricas_estado_tiempo{
    t_estado estado;
    int tiempo_inicio;
    int tiempo_fin;
    struct t_metricas_estado_tiempo* sig;
};
typedef struct t_metricas_estado_tiempo t_metricas_estado_tiempo;

// Control Block de un proceso
typedef struct {
    int pid;                    // Identificador único
    int program_counter;        // Contador de instrucción
    char* instrucciones;      // Lista de instrucciones (strings o structs)
    t_estado estado;            // Estado actual del proceso
    int tamanio_proceso; 
    t_metricas_cant* metricas_estado;         // Lista de veces que estuvo en cada estado
    t_metricas_estado_tiempo* metricas_tiempo;  // Lista de tiempo que estuvo en cada estado
    // Más campos opcionales: tamaño de memoria, registros, métricas, etc.
} t_pcb;


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


#endif // PCB_H_

