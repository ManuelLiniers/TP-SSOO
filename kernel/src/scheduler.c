#include "scheduler.h"

// Definición de las colas globales
t_queue* queue_new;
t_queue* queue_ready;
t_queue* queue_block;
t_queue* queue_exit;

void scheduler_init(void) {
    queue_new   = queue_create();
    queue_ready = queue_create();
    queue_block = queue_create();
    queue_exit  = queue_create();
}

void scheduler_destroy(void) {
    queue_destroy_and_destroy_elements(queue_new, pcb_destroy);
    queue_destroy_and_destroy_elements(queue_ready, pcb_destroy);
    queue_destroy_and_destroy_elements(queue_block, pcb_destroy);
    queue_destroy_and_destroy_elements(queue_exit, pcb_destroy);
}
