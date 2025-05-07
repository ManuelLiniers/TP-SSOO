#ifndef PCB_H_
#define PCB_H_

#include <commons/collections/list.h>

// Estados posibles de un proceso
typedef enum { NEW, READY, RUNNING, BLOCKED, EXIT } t_estado;

// Control Block de un proceso
typedef struct {
    int pid;                    // Identificador único
    int program_counter;        // Contador de instrucción
    t_list* instrucciones;      // Lista de instrucciones (strings o structs)
    t_estado estado;            // Estado actual del proceso
    //t_list* metricas_estado;         // Lista de veces que estuvo en cada estado
    //t_list* metricas_estado_tiempo;  // Lista de tiempo que estuvo en cada estado
    // Más campos opcionales: tamaño de memoria, registros, métricas, etc.
} t_pcb;

/**
 * @brief Crea un PCB inicializado
 * @param pid Identificador del proceso
 * @param instrucciones Lista de instrucciones (creada externamente)
 * @return puntero a t_pcb o NULL en caso de error
 */
t_pcb* pcb_create(int pid, t_list* instrucciones);

/**
 * @brief Destruye un PCB liberando instrucciones y la propia estructura
 * @param pcb_void puntero a t_pcb (pasado como void* para destruir en queues)
 */
void pcb_destroy(void* pcb_void);

#endif // PCB_H_

