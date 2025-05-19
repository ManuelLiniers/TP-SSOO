#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <commons/collections/queue.h>
#include "pcb.h"
#include <semaforos.h>

extern t_queue* queue_new;
extern t_queue* queue_ready;
extern t_queue* queue_block;
extern t_queue* queue_exit;

extern char* ip_memoria;
extern char* puerto_memoria;

/**
 * @brief Inicializa las cuatro colas del scheduler
 */
void scheduler_init(void);

/**
 * @brief Destruye las colas liberando también los PCBs que contienen
 */
void scheduler_destroy(void);

void* planificar_largo_plazo(void* arg);
void* planificar_corto_plazo(void* arg);

#endif // SCHEDULER_H_