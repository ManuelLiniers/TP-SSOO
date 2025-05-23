#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <commons/collections/queue.h>
#include "estructuras.h"
#include "semaforos.h"
#include "conexion.h"

extern t_queue* queue_new;
extern t_queue* queue_ready;
extern t_list* queue_block;
extern t_queue* queue_exit;


/**
 * @brief Inicializa las cuatro colas del scheduler
 */
void scheduler_init(void);

/**
 * @brief Destruye las colas liberando también los PCBs que contienen
 */
void scheduler_destroy(void);

void* planificar_largo_plazo(void* arg);
void esperar_devolucion_proceso(void* arg);
void esperar_dispatch(void* arg);
void esperar_interrupt(void* arg);
void* planificar_corto_plazo(void* arg);



#endif // SCHEDULER_H_