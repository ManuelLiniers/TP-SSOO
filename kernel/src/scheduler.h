#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <commons/collections/queue.h>
#include "pcb.h"

extern t_queue* queue_new;
extern t_queue* queue_ready;
extern t_queue* queue_block;
extern t_queue* queue_exit;

/**
 * @brief Inicializa las cuatro colas del scheduler
 */
void scheduler_init(void);

/**
 * @brief Destruye las colas liberando también los PCBs que contienen
 */
void scheduler_destroy(void);

#endif // SCHEDULER_H_