#include "scheduler.h"

void cambiarEstado(t_pcb* proceso,t_estado EXEC);
bool espacio_en_memoria(t_pcb* proceso);
void poner_en_ejecucion(t_pcb* proceso);

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
        wait_sem(&proceso_ready);
        // wait de cpu libre?
        wait_mutex(&mutex_queue_ready);
        if(!queue_is_empty(queue_ready)){
            t_pcb* proceso = queue_pop(queue_ready);
            signal_mutex(&mutex_queue_ready);
            poner_en_ejecucion(proceso);
            cambiarEstado(proceso, EXEC);
            /* 
            crear hilo que quede a la espera de recibir dicho PID después de la ejecución? 
            */
           /* pthread_t* esperar_devolucion = malloc(sizeof(pthread_t));
           pthread_create(esperar_devolucion, NULL, (void*) esperar_devolucion_proceso, NULL); */
        }
        else{
            signal_mutex(&mutex_queue_ready);
        }
	}

}

/* void esperar_devolucion_proceso(void* arg){
    t_buffer* buffer = malloc(sizeof(t_buffer));
    recv(socket, buffer, , NULL);

} */

void* planificar_largo_plazo(void* arg){
	while(1){
        wait_sem(&nuevo_proceso);
        wait_mutex(&mutex_queue_new);
		if(!queue_is_empty(queue_new)){
			t_pcb* proceso = queue_peek(queue_new);
            signal_mutex(&mutex_queue_new);
			if(espacio_en_memoria(proceso)){

                wait_mutex(&mutex_queue_new);
				queue_pop(queue_new);
                cambiarEstado(proceso, READY);
                signal_mutex(&mutex_queue_new);

                wait_mutex(&mutex_queue_ready);
				queue_push(queue_ready, proceso);
                signal_mutex(&mutex_queue_ready);

                signal_sem(&proceso_ready);
			}
            else{
                signal_sem(&nuevo_proceso); // el proceso nuevo sigue en NEW, hago signal de vuelta
                wait_sem(&proceso_terminado); // espero que algun proceso finalice 

                // HACER SIGNAL EN FINALIZACION
            }
		}
		else{
            signal_mutex(&mutex_queue_new); // libero recurso del mutex
		}
	}

}

bool espacio_en_memoria(t_pcb* proceso){
    int conexion = crear_conexion_memoria();
    hacer_handshake(logger_kernel, conexion);

    t_paquete* paqueteID = crear_paquete(IDENTIFICACION);
    agregar_a_paquete(paqueteID, KERNEL, sizeof(op_code));
    enviar_paquete(paqueteID, conexion);

    t_paquete* paqueteInfo = crear_paquete(INICIAR_PROCESO);
    agregar_a_paquete(paqueteInfo, &proceso->pid, sizeof(int));
    agregar_a_paquete(paqueteInfo, &proceso->tamanio_proceso, sizeof(int));
    agregar_a_paquete(paqueteInfo, &proceso->instrucciones, sizeof(char*));
    enviar_paquete(paqueteInfo, conexion);

    // falta recibir y analizar respuesta de memoria

    t_buffer* buffer = malloc(sizeof(t_buffer));
    int resultado = recv(conexion, buffer, sizeof(int), 0);
    free(buffer);
    return resultado==OK;
}


void cambiarEstado(t_pcb* proceso, t_estado estado){

    proceso->estado = estado;
    proceso->metricas_estado[id_estado(estado)]++;

    // falta las metricas del tiempo

    t_metricas_estado_tiempo* metrica = malloc(sizeof(t_metricas_estado_tiempo));
    metrica->estado = estado;
    // metrica->tiempo_inicio = tiempo_ahora;

    list_add(proceso->metricas_tiempo, metrica);
}

void poner_en_ejecucion(t_pcb* proceso){

    // mandar a alguna cpu dependiendo de cual este libre

}