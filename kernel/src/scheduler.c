#include "scheduler.h"

void cambiarEstado(t_pcb* proceso,t_estado EXEC);
bool espacio_en_memoria(t_pcb* proceso);
void poner_en_ejecucion(t_pcb* proceso, t_cpu** cpu_encargada);
t_dispositivo_io* buscar_io(int id_io);
t_cpu* buscar_cpu_libre(t_list* lista_cpus);

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
            t_cpu* cpu_encargada = malloc(sizeof(t_cpu));
            poner_en_ejecucion(proceso, &cpu_encargada);
            cambiarEstado(proceso, EXEC);
            /* 
            crear hilo que quede a la espera de recibir dicho PID después de la ejecución? 
            */
           pthread_t* esperar_devolucion = malloc(sizeof(pthread_t));
           pthread_create(esperar_devolucion, NULL, (void*) esperar_devolucion_proceso, (void*) cpu_encargada);
        }
        else{
            signal_mutex(&mutex_queue_ready);
        }
	}

}

void esperar_devolucion_proceso(void* arg){
    pthread_t* dispatch = malloc(sizeof(pthread_t));
    pthread_create(dispatch, NULL, (void*) esperar_dispatch, arg);
    pthread_t* interrupt = malloc(sizeof(pthread_t));
    pthread_create(interrupt, NULL, (void*) esperar_interrupt, arg);
}

void esperar_dispatch(void* arg){
    t_cpu* cpu_encargada = (t_cpu*) arg;
    t_buffer* buffer = malloc(sizeof(t_buffer)); 
    recv(cpu_encargada->socket_dispatch, buffer, sizeof(int), 0); 
    
    /* 
    
    CORREGIR sizeof(int) 
    
    */

    free(buffer);
}

void esperar_interrupt(void* arg){
    t_cpu* cpu_encargada = (t_cpu*) arg;
    t_buffer* buffer = malloc(sizeof(t_buffer)); 
    recv(cpu_encargada->socket_interrupt, buffer, sizeof(int), 0);
    
    /* 
    
    CORREGIR sizeof(int) 
    
    */

    int* pid_proceso = malloc(sizeof(int));
    memcpy(buffer->stream, (void*) pid_proceso, sizeof(int));
    int* tiempo_proceso = malloc(sizeof(int));
    memcpy(buffer->stream, (void*) tiempo_proceso, sizeof(int));
    int* id_io_a_usar = malloc(sizeof(int));
    memcpy(buffer->stream, (void*) id_io_a_usar, sizeof(int));
    int id_io = *id_io_a_usar;

    t_dispositivo_io* dispositivo = buscar_io(id_io);
    if(dispositivo == NULL){
        log_error(logger_kernel, "No se encontro la IO: %d", id_io);
    }
    t_paquete* paquete = crear_paquete(PETICION_IO);
    agregar_a_paquete(paquete, pid_proceso, sizeof(int));
    agregar_a_paquete(paquete, tiempo_proceso,sizeof(int));
    enviar_paquete(paquete, dispositivo->socket);
    eliminar_paquete(paquete);
    free(buffer);
}

t_dispositivo_io* buscar_io(int id_io){
    for(int i = 0; i<list_size(lista_dispositivos_io); i++){
        t_dispositivo_io* actual = (t_dispositivo_io*) list_get(lista_dispositivos_io, i);
        if(actual->id == id_io){
            return actual;
        }
    }
    return NULL;
}


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
    eliminar_paquete(paqueteID);

    t_paquete* paqueteInfo = crear_paquete(INICIAR_PROCESO);
    agregar_a_paquete(paqueteInfo, &proceso->pid, sizeof(int));
    agregar_a_paquete(paqueteInfo, &proceso->tamanio_proceso, sizeof(int));
    agregar_a_paquete(paqueteInfo, &proceso->instrucciones, sizeof(char*));
    enviar_paquete(paqueteInfo, conexion);

    eliminar_paquete(paqueteInfo);
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

void poner_en_ejecucion(t_pcb* proceso, t_cpu** cpu_encargada){

    t_cpu* cpu_libre = buscar_cpu_libre(lista_cpus);
    // mandar a alguna cpu dependiendo de cual este libre

    t_paquete* paquete = crear_paquete(CONTEXTO_PROCESO);
    agregar_a_paquete(paquete, &proceso->pid, sizeof(int));
    agregar_a_paquete(paquete, &proceso->program_counter, sizeof(int));
    enviar_paquete(paquete, cpu_libre->socket_dispatch);
    eliminar_paquete(paquete);

    //
    *cpu_encargada = cpu_libre;

}

t_cpu* buscar_cpu_libre(t_list* lista_cpus){
    for(int i = 0; i<list_size(lista_cpus); i++){
        t_cpu* actual = list_get(lista_cpus, i);
        if(actual->esta_libre){
            return actual;
        }
    }
    log_error(logger_kernel, "No se hay CPUs libres");
    return NULL;
}