#include "scheduler.h"

char* algoritmo_corto_plazo;
char* algoritmo_largo_plazo;

void *planificar_corto_plazo_FIFO(void* arg){
    log_debug(logger_kernel, "Entre a Corto Plazo FIFO");
	while(1){
        wait_sem(&proceso_ready);
        wait_sem(&cpu_libre);
        if(!list_is_empty(queue_ready)){
            log_debug(logger_kernel, "Busco una CPU libre");
            t_cpu* cpu_encargada = NULL;
            if(hay_cpu_libre(&cpu_encargada)){
                wait_mutex(&mutex_queue_ready);
                t_pcb* proceso = (t_pcb*) list_remove(queue_ready, 0);
                signal_mutex(&mutex_queue_ready);
                poner_en_ejecucion(proceso, cpu_encargada, cpu_encargada->socket_dispatch);
                pthread_t esperar_devolucion;
                pthread_create(&esperar_devolucion, NULL, esperar_dispatch, (void*) cpu_encargada);
                pthread_detach(esperar_devolucion);
                // free(cpu_encargada);
            }
            else{
                signal_sem(&proceso_ready);
            }
        }
	}

}

bool hay_cpu_libre(t_cpu** cpu_encargada){
    t_cpu* cpu_libre = buscar_cpu_libre(lista_cpus);
    *cpu_encargada = cpu_libre;

    return cpu_libre != NULL;
}

void* esperar_dispatch(void* arg){
    t_cpu* cpu_encargada = (t_cpu*) arg;

    if(CONTEXTO_PROCESO != recibir_operacion(cpu_encargada->socket_dispatch)){
        log_error(logger_kernel, "Se esperaba recibir CONTEXTO_PROCESO");
        return NULL;
    }
    t_buffer* paquete = recibir_paquete(cpu_encargada->socket_dispatch);

    uint32_t pid = recibir_uint32_del_buffer(paquete);
    t_pcb* proceso = buscar_proceso_pid(pid);
    uint32_t pc = recibir_uint32_del_buffer(paquete);
    int motivo = recibir_int_del_buffer(paquete);

    log_debug(logger_kernel, "Motivo: %d", motivo);

    proceso->program_counter = pc + 1;
    
    switch (motivo)
    {
    case FINALIZADO:
        cambiar_estado(proceso, EXIT);

        wait_mutex(&mutex_queue_exit);
        queue_push(queue_exit, proceso);
        signal_mutex(&mutex_queue_exit);

        sacar_proceso_ejecucion(proceso);
        liberar_cpu(cpu_encargada);
        //list_add(lista_cpus, cpu_encargada);

        log_info(logger_kernel, "## (<%d>) - Solicito syscall: <%s>", proceso->pid, "EXIT");

        log_info(logger_kernel, "## %d - Finaliza el proceso", proceso->pid);
        
        log_metricas_estado(proceso);
        paquete_memoria_pid(proceso, FINALIZAR_PROCESO);
        signal_sem(&espacio_memoria);

    //
//              ############# AVISAR A SEMAFOROS PARA QUE SIGA CORRIENDO EL PROCESO #############
    //
        //signal_sem(&proceso_ready);
        //free(proceso);
        break;
    case CAUSA_IO:
        int tamanio = recibir_int_del_buffer(paquete);
        char* nombre_io = recibir_informacion_del_buffer(paquete, tamanio);
        int io_tiempo = recibir_int_del_buffer(paquete);

        tiempo_en_io* proceso_bloqueado = malloc(sizeof(t_dispositivo_io));
        proceso_bloqueado->pcb = proceso;
        proceso_bloqueado->tiempo = io_tiempo;

        sacar_proceso_ejecucion(proceso);
        liberar_cpu(cpu_encargada);
        //list_add(lista_cpus, cpu_encargada);

        t_dispositivo_io* dispositivo = buscar_io_libre(nombre_io);
        if(dispositivo != NULL){
            enviar_proceso_a_io(proceso, dispositivo, io_tiempo);
        }
        else{
            dispositivo = buscar_io_menos_ocupada(nombre_io);
        }

        if(dispositivo == NULL){
            log_error(logger_kernel, "No se encontro la IO: %s", nombre_io);
            break;
        }

        wait_mutex(&mutex_queue_block);
        queue_push(obtener_cola_io(dispositivo->id), proceso_bloqueado);
        signal_mutex(&mutex_queue_block);

        log_debug(logger_kernel, "Proceso <%d> en dispositivo %s: ",proceso->pid ,dispositivo->nombre);

        cambiar_estado(proceso, BLOCKED);

        pthread_t comprobacion_suspendido;
        pthread_create(&comprobacion_suspendido, NULL, (void*) comprobar_suspendido, proceso);
        pthread_detach(comprobacion_suspendido);
        break;

    case INIT_PROC:
        int tamanio_proceso = recibir_int_del_buffer(paquete);
        int longitud = recibir_int_del_buffer(paquete);
        char* archivo = recibir_informacion_del_buffer(paquete, longitud);
        //char* nombre = malloc(sizeof(char));
        //sprintf(nombre, "%d",tamanio_proceso);

        log_info(logger_kernel, "## (<%d>) - Solicito syscall: <%s>", proceso->pid, "INIT_PROC");

        sacar_proceso_ejecucion(proceso);
        liberar_cpu(cpu_encargada);
        //list_add(lista_cpus, cpu_encargada);

        poner_en_ready(proceso);

        crear_proceso(archivo, tamanio_proceso);

    //
//              ############# AVISAR A SEMAFOROS PARA QUE SIGA CORRIENDO EL PROCESO #############
    //
        //signal_sem(&proceso_ready);
        //crear_proceso(archivo, nombre);
        //free(nombre);
        break;
    case MEMORY_DUMP:
        bool resultado_dump = paquete_memoria_pid(proceso, DUMP_MEMORY); 

        sacar_proceso_ejecucion(proceso);
        liberar_cpu(cpu_encargada);
        if(resultado_dump){
            poner_en_ready(proceso);
        } else {
            cambiar_estado(proceso, EXIT);

            wait_mutex(&mutex_queue_exit);
            queue_push(queue_exit, proceso);
            signal_mutex(&mutex_queue_exit);

            log_info(logger_kernel, "## %d - Finaliza el proceso", proceso->pid);

            log_metricas_estado(proceso);
            paquete_memoria_pid(proceso, FINALIZAR_PROCESO);
            signal_sem(&espacio_memoria);
        }
        break;
    default:
        log_error(logger_kernel, "Motivo de desalojo desconocido");
        break;
    }
    free(paquete);
    return NULL;
}

bool paquete_memoria_pid(t_pcb* proceso, op_code codigo){
    int conexion = crear_conexion_memoria();
    
    t_paquete* paquete = crear_paquete(codigo);
    agregar_a_paquete(paquete, &(proceso->pid), sizeof(int));
    enviar_paquete(paquete, conexion);
    eliminar_paquete(paquete);

    int resultado;
    recv(conexion, &resultado, sizeof(int), 0);
    liberar_conexion(conexion);
    return resultado == OK;
}

void* planificar_corto_plazo_SJF(void* arg){
    while(1){
        wait_sem(&proceso_ready);
        wait_mutex(&mutex_queue_ready);
        if(!list_is_empty(queue_ready)){
            list_sort(queue_ready, shortest_job_first);
            t_cpu* cpu_encargada = malloc(sizeof(t_cpu));
            if(hay_cpu_libre(&cpu_encargada)){
                t_pcb* proceso = list_remove(queue_ready, 0);
                signal_mutex(&mutex_queue_ready);
                poner_en_ejecucion(proceso, cpu_encargada, cpu_encargada->socket_dispatch);
                pthread_t esperar_devolucion;
                pthread_create(&esperar_devolucion, NULL, (void*) esperar_dispatch, (void*) cpu_encargada);
                pthread_detach(esperar_devolucion);
                //free(cpu_encargada);
            }
            signal_mutex(&mutex_queue_ready);
        }
        else{
            signal_mutex(&mutex_queue_ready);
        }
    }

}

void* planificar_corto_plazo_SJF_desalojo(void* arg){
      while(1){
        wait_sem(&proceso_ready);
        wait_mutex(&mutex_procesos_ejecutando);
        actualizar_estimaciones();
        signal_mutex(&mutex_procesos_ejecutando);
        wait_mutex(&mutex_queue_ready);
        if(!list_is_empty(queue_ready)){
            list_sort(queue_ready, shortest_job_first);
            t_cpu* cpu_encargada = malloc(sizeof(t_cpu));
            if(hay_cpu_libre(&cpu_encargada)){
                t_pcb* proceso = list_remove(queue_ready, 0);
                signal_mutex(&mutex_queue_ready);
                poner_en_ejecucion(proceso, cpu_encargada, cpu_encargada->socket_dispatch);
                cambiar_estado(proceso, EXEC);
                pthread_t esperar_devolucion;
                pthread_create(&esperar_devolucion, NULL, (void*) esperar_dispatch, (void*) cpu_encargada);
                pthread_detach(esperar_devolucion);
                //free(cpu_encargada);
            }
            else{
                signal_mutex(&mutex_queue_ready);
            }
            /*  
                Hay que ir iterando la lista de procesos en ready para ver si hay procesos en ejecucion
                que tienen una estimacion mas grande que los que estan en ready. Si es asi hay que desalojarlos
            */ 

            wait_mutex(&mutex_queue_ready);
            list_sort(queue_ready, shortest_job_first);
            t_pcb* proceso_prueba = list_get(queue_ready, 0);
            signal_mutex(&mutex_queue_ready);

            for(int i = 0; i<list_size(lista_procesos_ejecutando); i++){
                t_unidad_ejecucion* proceso_ejecutando = list_get(lista_procesos_ejecutando, i);
                if(shortest_job_first((void*) proceso_prueba, (void*) proceso_ejecutando->proceso)){
                    wait_mutex(&mutex_queue_ready);
                    list_remove_element(queue_ready, proceso_prueba);
                    signal_mutex(&mutex_queue_ready);
                    sacar_proceso_ejecucion(proceso_ejecutando->proceso);
                    poner_en_ejecucion(proceso_prueba, proceso_ejecutando->cpu, proceso_ejecutando->cpu->socket_interrupt);
                    log_info(logger_kernel, "## (<%d>) - Desalojado por algoritmo SJF/SRT", proceso_ejecutando->proceso->pid);
                    list_add(queue_ready, proceso_ejecutando->proceso);
                }
            }
            /* int cant_procesos;
            for(int i = 0; i<list_size(queue_ready);i++){
                t_pcb* otro_proceso = list_get(queue_ready, i);
                for(int j = 0; j<list_size(lista_procesos_ejecutando); j++){
                    t_unidad_ejecucion* proceso_ejecutando = list_get(lista_procesos_ejecutando, j);
                    if(shortest_job_first((void*) otro_proceso, (void*) proceso_ejecutando->proceso) && otro_proceso->estado == READY){
                        poner_en_ejecucion(otro_proceso, proceso_ejecutando->cpu, proceso_ejecutando->cpu->socket_interrupt);
                        cant_procesos++;
                        sacar_proceso_ejecucion(proceso_ejecutando->proceso);
                        log_info(logger_kernel, "## (<%d>) - Desalojado por algoritmo SJF/SRT", proceso_ejecutando->proceso->pid);
                        list_add_sorted(queue_ready, proceso_ejecutando, shortest_job_first);
                    }
                }

            }
            for(int i=0; i<cant_procesos ; i++){
                list_remove(queue_ready, i);
            } */
        }
        else{
            signal_mutex(&mutex_queue_ready);
        }
    }
}

void actualizar_estimaciones(){
    for(int i = 0; i<list_size(lista_procesos_ejecutando); i++){
        t_unidad_ejecucion* ejecucion = list_get(lista_procesos_ejecutando, i);
        t_pcb* proceso = ejecucion->proceso;
        proceso->estimacion_actual -= temporal_gettime(ejecucion->tiempo_ejecutando);
    }
}

void log_metricas_estado(t_pcb* proceso){
    long tiempo_new = 0;
    long tiempo_ready = 0;
    long tiempo_exec = 0;
    long tiempo_blocked = 0;
    long tiempo_susp_ready = 0;
    long tiempo_susp_blocked = 0;
    for(int i = 0; i<list_size(proceso->metricas_tiempo);i++){
        t_metricas_estado_tiempo* metrica = list_get(proceso->metricas_tiempo, i);
        long tiempo = calcular_tiempo(metrica);
        switch (metrica->estado)
        {
        case NEW:
            tiempo_new +=  tiempo;
            break;
        case READY:
            tiempo_ready  +=  tiempo;
            break;
        case EXEC:
            tiempo_exec +=  tiempo;
            break;
        case BLOCKED:
            tiempo_blocked +=  tiempo;
            break;
        case SUSP_READY:
            tiempo_susp_ready +=  tiempo;
            break;
        case SUSP_BLOCKED:
            tiempo_susp_blocked +=  tiempo;
            break;
        case EXIT:
            break;
        default:
            break;
        }

    }

    log_info(logger_kernel, "## %d - Metricas de estado: NEW %d %ld, READY %d %ld, EXEC %d %ld, BLOCKED %d %ld, SUSP_READY %d %ld, SUSP_BLOCKED %d %ld", 
    proceso->pid, proceso->metricas_estado[0], tiempo_new, proceso->metricas_estado[1], tiempo_ready, proceso->metricas_estado[2], tiempo_exec, 
    proceso->metricas_estado[3], tiempo_blocked, proceso->metricas_estado[4], tiempo_susp_ready, proceso->metricas_estado[5], tiempo_susp_blocked);

}


long calcular_tiempo(t_metricas_estado_tiempo* metrica){
    char* tiempo_inicio = metrica->tiempo_inicio;
    char* tiempo_fin = metrica->tiempo_fin;
    
    return obtener_diferencia_tiempo(tiempo_fin, tiempo_inicio);
}



long obtener_diferencia_tiempo(char* tiempo_1, char* tiempo_2){
    int h1, m1, s1, ms1;
    int h2, m2, s2, ms2;

    sscanf(tiempo_1, "%d:%d:%d:%d", &h1, &m1, &s1, &ms1);
    sscanf(tiempo_2, "%d:%d:%d:%d", &h2, &m2, &s2, &ms2);

    long total_ms1 = ((h1 * 3600) + (m1 * 60) + s1) * 1000 + ms1;
    long total_ms2 = ((h2 * 3600) + (m2 * 60) + s2) * 1000 + ms2;

    long diff_ms = total_ms1 - total_ms2;

    return diff_ms;
}


void *planificar_largo_plazo_FIFO(void* arg){
	while(1){
        wait_sem(&nuevo_proceso);
        wait_mutex(&mutex_queue_susp_ready);
        if(!queue_is_empty(queue_susp_ready)){
            tiempo_en_io* proceso_a_desuspender = queue_peek(queue_susp_ready);
            t_pcb* proceso = proceso_a_desuspender->pcb;
            signal_mutex(&mutex_queue_susp_ready);
            if(vuelta_swap(proceso)){

                wait_mutex(&mutex_queue_susp_ready);
				queue_pop(queue_susp_ready);
                signal_mutex(&mutex_queue_susp_ready);

                // PONER EN READY
                poner_en_ready(proceso);
                log_debug(logger_kernel, "Cola de ready:");
                mostrar_lista(queue_ready);
			}
            else{
                signal_sem(&nuevo_proceso); // el proceso nuevo sigue en NEW, hago signal de vuelta
                wait_sem(&espacio_memoria); // espero que algun proceso finalice 
            }
        }
        else{
            signal_mutex(&mutex_queue_susp_ready);
            wait_mutex(&mutex_queue_new);
            if(!list_is_empty(queue_new)){
			t_pcb* proceso = list_get(queue_new, 0);
            signal_mutex(&mutex_queue_new);
			if(espacio_en_memoria(proceso)){

                wait_mutex(&mutex_queue_new);
				list_remove_element(queue_new, proceso);
                signal_mutex(&mutex_queue_new);

                // PONER EN READY
                poner_en_ready(proceso);
                log_debug(logger_kernel, "Cola de ready:");
                mostrar_lista(queue_ready);
			}
            else{
                signal_sem(&nuevo_proceso); // el proceso nuevo sigue en NEW, hago signal de vuelta
                wait_sem(&espacio_memoria); // espero que algun proceso finalice 
            }
            signal_mutex(&mutex_queue_new);
		}
        }
	}

}

void *planificar_largo_plazo_PMCP(void* arg){
    pthread_t hilo_comprobar_espacio;
    pthread_create(&hilo_comprobar_espacio, NULL, (void*) comprobar_procesos_nuevos, arg);
    pthread_detach(hilo_comprobar_espacio);

    while(1){
        wait_sem(&espacio_memoria);
        if(!queue_is_empty(queue_susp_ready)){
            wait_mutex(&mutex_queue_susp_ready);
            t_pcb* proceso = queue_peek(queue_susp_ready);
            signal_mutex(&mutex_queue_susp_ready);
            if(espacio_en_memoria(proceso)){

                wait_mutex(&mutex_queue_susp_ready);
				queue_pop(queue_susp_ready);
                signal_mutex(&mutex_queue_susp_ready);
                // PONER EN READY
                poner_en_ready(proceso);
                log_debug(logger_kernel, "Cola de ready:");
                mostrar_lista(queue_ready);
			}
        }
        else{
            wait_mutex(&mutex_queue_new);
            list_sort(queue_new, proceso_es_mas_chico);
            t_pcb *proceso = list_get(queue_new, 0);
            signal_mutex(&mutex_queue_new);
            if(espacio_en_memoria(proceso)){
                wait_mutex(&mutex_queue_new);
                list_remove_element(queue_new, proceso);
                signal_mutex(&mutex_queue_new);

                // PONER EN READY
                log_debug(logger_kernel, "Cola de ready:");
                mostrar_lista(queue_ready);
                poner_en_ready(proceso);
        }
        }
    }
}

void *comprobar_procesos_nuevos(void* arg){
    while(1){
        wait_sem(&nuevo_proceso);

        wait_mutex(&mutex_queue_new);
        list_sort(queue_new, proceso_es_mas_chico);
        t_pcb* proceso = list_get(queue_new, 0);
        signal_mutex(&mutex_queue_new);

        if(queue_is_empty(queue_susp_ready) && espacio_en_memoria(proceso)){

            wait_mutex(&mutex_queue_new);
            list_remove_element(queue_new, proceso);
            signal_mutex(&mutex_queue_new);

            poner_en_ready(proceso);
        }
    }
}

bool espacio_en_memoria(t_pcb* proceso){
   int conexion = crear_conexion_memoria();

    t_paquete* paqueteInfo = crear_paquete(INICIAR_PROCESO);
    agregar_a_paquete(paqueteInfo, &proceso->pid, sizeof(int));
    agregar_a_paquete(paqueteInfo, &proceso->tamanio_proceso, sizeof(int));
    int longitud = strlen(proceso->instrucciones) + 1;
    agregar_a_paquete(paqueteInfo, &longitud, sizeof(int));
    agregar_a_paquete(paqueteInfo, proceso->instrucciones, longitud);
    enviar_paquete(paqueteInfo, conexion);

    eliminar_paquete(paqueteInfo);

    int resultado;
    recv(conexion, &resultado, sizeof(int), 0);
    //close(conexion);
    if(resultado == OK){
        int avisoCreado;
        recv(conexion, &avisoCreado, sizeof(int), 0);
        return true;
    }
    else{
        return false;
    }
    //return 1;
}

bool vuelta_swap(t_pcb* proceso){
    int conexion = crear_conexion_memoria();
    t_paquete* paquete = crear_paquete(VUELTA_SWAP);
    agregar_a_paquete(paquete, &proceso->pid, sizeof(int));
    enviar_paquete(paquete, conexion);
    eliminar_paquete(paquete);

    int resultado;
    recv(conexion, &resultado, sizeof(int), 0);
    return resultado == OK;
}

void poner_en_ready(t_pcb* proceso){
    cambiar_estado(proceso, READY);
    wait_mutex(&mutex_queue_ready);
	list_add(queue_ready, proceso);
    signal_mutex(&mutex_queue_ready);
    signal_sem(&proceso_ready);
}

bool proceso_es_mas_chico(void* a, void* b){
    t_pcb* proceso_a = (t_pcb*) a;
    t_pcb* proceso_b = (t_pcb*) b;
    

    return proceso_a->tamanio_proceso <= proceso_b->tamanio_proceso;
}

bool shortest_job_first(void* a, void* b){
    t_pcb* proceso_a = (t_pcb*) a;
    t_pcb* proceso_b = (t_pcb*) b;
    
    return proceso_a->estimacion_actual < proceso_b->estimacion_actual;
}
void poner_en_ejecucion(t_pcb* proceso, t_cpu* cpu_encargada, int socket){
    t_paquete* paquete = crear_paquete(CONTEXTO_PROCESO);
    agregar_a_paquete(paquete, &proceso->pid, sizeof(int));
    agregar_a_paquete(paquete, &proceso->program_counter, sizeof(int));
    // enviar_paquete(paquete, cpu_libre->socket_dispatch);
    enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);

    cpu_encargada->esta_libre = 0;
    t_unidad_ejecucion* nueva = malloc(sizeof(t_unidad_ejecucion));
    nueva->cpu = cpu_encargada;
    nueva->proceso = proceso;
    nueva->tiempo_ejecutando = temporal_create();
    list_add(lista_procesos_ejecutando, nueva);
    cambiar_estado(proceso, EXEC);
}

void enviar_proceso_a_io(t_pcb* proceso, t_dispositivo_io* dispositivo, int io_tiempo){
    
    t_paquete* paquete = crear_paquete(PETICION_IO);
    agregar_a_paquete(paquete, &(proceso->pid), sizeof(int));
    agregar_a_paquete(paquete, &io_tiempo,sizeof(int));
    enviar_paquete(paquete, dispositivo->socket);

    log_info(logger_kernel, "## %d - Bloqueado por IO: %s", proceso->pid, dispositivo->nombre);

    /* 
    pthread_t hilo_vuelta_io;
    pthread_create(&hilo_vuelta_io, NULL, (void*) vuelta_proceso_io, dispositivo); // creo hilo para esperar la vuelta de la io
    pthread_detach(hilo_vuelta_io); 
    */
}

void vuelta_proceso_io(void* args){
    t_dispositivo_io* io = (t_dispositivo_io*) args;

    int respuesta;
    recv(io->socket, &respuesta, sizeof(int), 0);
    if(respuesta == FINALIZACION_IO){
        tiempo_en_io *proceso = queue_pop(obtener_cola_io(io->id));

        if(proceso->pcb->estado == SUSP_BLOCKED){
            wait_mutex(&mutex_queue_susp_ready);
            queue_push(queue_susp_ready, proceso);
            signal_mutex(&mutex_queue_susp_ready);
            cambiar_estado(proceso->pcb, SUSP_READY);
        }
        else{
            poner_en_ready(proceso->pcb);
        }

        log_info(logger_kernel, "## %d finalizó IO", proceso->pcb->pid);

        comprobar_cola_bloqueados(io);
    }
    else{
        log_debug(logger_kernel, "Codigo de vuelta de io incorrecto");
    }
}

void comprobar_suspendido(t_pcb* proceso){
    usleep(tiempo_suspension / 1000); // tiempo_suspension en ms / 1000 = tiempo_suspension en us
    if(proceso->estado == BLOCKED){
        cambiar_estado(proceso, SUSP_BLOCKED);
        paquete_memoria_pid(proceso, SWAP);
        signal_sem(&espacio_memoria);
    }
}

/* void verificar_procesos_ready(){
    wait_mutex(&mutex_queue_susp_ready);
    if(!queue_is_empty(queue_susp_ready)){
        t_pcb* proceso = queue_peek(queue_susp_ready);
        signal_mutex(&mutex_queue_susp_ready);
        if(espacio_en_memoria(proceso)){
            wait_mutex(&mutex_queue_susp_ready);
			queue_pop(queue_susp_ready);
            signal_mutex(&mutex_queue_susp_ready);
            // PONER EN READY
            poner_en_ready(proceso);
            log_debug(logger_kernel, "Cola de ready:");
            mostrar_lista(queue_ready);
        }
    }
    else{
        wait_mutex(&mutex_queue_new);
        if(!list_is_empty(queue_new)){
            t_pcb* proceso = list_get(queue_new, 0);
            signal_mutex(&mutex_queue_new);
            if(espacio_en_memoria(proceso)){
                wait_mutex(&mutex_queue_new);
                list_remove_element(queue_new, proceso);
                signal_mutex(&mutex_queue_new);
                // PONER EN READY
                poner_en_ready(proceso);
                log_debug(logger_kernel, "Cola de ready:");
                mostrar_lista(queue_ready);                
            }
        }
    }
} */

void comprobar_cola_bloqueados(t_dispositivo_io* dispositivo){
    wait_mutex(&mutex_queue_block);
    t_queue* cola_io = obtener_cola_io(dispositivo->id);
    if(!queue_is_empty(cola_io)){
        tiempo_en_io *proceso = queue_peek(cola_io);
        signal_mutex(&mutex_queue_block);

        enviar_proceso_a_io(proceso->pcb, dispositivo, proceso->tiempo);
    }
    else{
        signal_mutex(&mutex_queue_block);
    }
}