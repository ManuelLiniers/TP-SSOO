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

void* planificar_corto_plazo(void* arg){
	while(1){
        t_pcb* proceso = queue_pop(queue_ready);
        cambiarEstado(proceso, EXEC);
        
        // enviar a CPU


	}

}

void* planificar_largo_plazo(void* arg){
	while(1){
        wait_sem(&nuevo_proceso);
        wait_mutex(&mutex_queue_new);
		if(!queue_is_empty(queue_new)){
			t_pcb* proceso = queue_peek(queue_new);
            signal_mutex(mutex_queue_new);
			if(espacio_en_memoria(proceso)){

                wait_mutex(mutex_queue_new);
				queue_pop(queue_new);
                signal_mutex(mutex_queue_new);

                wait_mutex(mutex_queue_ready);
				queue_push(queue_ready, proceso);
                signal_mutex(mutex_queue_ready);
			}
            else{
                signal_sem(&nuevo_proceso); // el proceso nuevo sigue en NEW, hago signal de vuelta
                wait_sem(&proceso_terminado); // espero que algun proceso finalice 

                //HACER SIGNAL EN FINALIZACION
            }
		}
		else{
            signal_mutex(mutex_queue_new); // libero recurso del mutex
		}
	}

}

bool espacio_en_memoria(*t_pcb proceso){
    
    // mandar el proceso a memoria

	return 1;
}

void cambiarEstado(t_pcb* proceso, t_estado estado){

    proceso->estado = estado;
    t_metricas_cant* metricas = proceso->metricas_estado;
    for(; metricas->estado != estado ; metricas = metricas->sig);
    metricas->cant++;

    // falta las metricas del tiempo
}