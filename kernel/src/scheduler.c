#include "scheduler.h"

void *planificar_corto_plazo_FIFO(void* arg){
	while(1){
        wait_sem(&proceso_ready);
        wait_sem(&cpu_libre);
        wait_mutex(&mutex_queue_ready);
        wait_mutex(&mutex_lista_cpus);
        wait_mutex(&mutex_procesos_ejecutando);
        wait_mutex(&mutex_pcb);
        if(!list_is_empty(queue_ready)){
            log_debug(logger_kernel, "Busco una CPU libre");
            t_cpu* cpu_encargada = NULL;
            if(hay_cpu_libre(&cpu_encargada)){
                t_pcb* proceso = (t_pcb*) list_remove(queue_ready, 0);
                poner_en_ejecucion(proceso, cpu_encargada);
            }
            else{
                signal_sem(&proceso_ready);
            }
        }
        signal_mutex(&mutex_pcb);
        signal_mutex(&mutex_lista_cpus);
        signal_mutex(&mutex_procesos_ejecutando);
        signal_mutex(&mutex_queue_ready);
	}
}

void* planificar_corto_plazo_SJF(void* arg){
    while(1){
        wait_sem(&proceso_ready);
        wait_sem(&cpu_libre);
        wait_mutex(&mutex_queue_ready);
        wait_mutex(&mutex_lista_cpus);
        wait_mutex(&mutex_procesos_ejecutando);
        wait_mutex(&mutex_pcb);
        if(!list_is_empty(queue_ready)){
            list_sort(queue_ready, shortest_job_first);
            mostrar_lista(queue_ready);
            t_cpu* cpu_encargada = NULL;
            if(hay_cpu_libre(&cpu_encargada)){
                t_pcb* proceso = list_remove(queue_ready, 0);
                //signal_mutex(&mutex_queue_ready);
                poner_en_ejecucion(proceso, cpu_encargada);
                //free(cpu_encargada);
            }
        }
        signal_mutex(&mutex_pcb);
        signal_mutex(&mutex_lista_cpus);
        signal_mutex(&mutex_queue_ready);     
        signal_mutex(&mutex_procesos_ejecutando);
    }
}

void* planificar_corto_plazo_SJF_desalojo(void* arg){
    //wait_mutex(&check_desalojo);

    pthread_t hilo_comprobar_desalojo;
    pthread_create(&hilo_comprobar_desalojo, NULL, (void*)comprobar_desalojo, NULL);
    pthread_detach(hilo_comprobar_desalojo);
    
    while(1){
        //int esperar_cpu = false;
        wait_sem(&planificacion_principal);
        wait_sem(&proceso_ready);
        wait_sem(&cpu_libre);
        wait_mutex(&mutex_queue_ready);
        wait_mutex(&mutex_lista_cpus);
        wait_mutex(&mutex_procesos_ejecutando);
        wait_mutex(&mutex_pcb);
        actualizar_estimaciones();
        log_debug(logger_kernel, "entro a revisar planificacion principal");
        if(!list_is_empty(queue_ready)){
            list_sort(queue_ready, shortest_job_first);
            //signal_mutex(&mutex_queue_ready);
            //mostrar_lista(queue_ready);
            t_cpu* cpu_encargada = NULL;

            if(hay_cpu_libre(&cpu_encargada)){
                //wait_sem(&cpu_libre);
                //wait_sem(&desalojando);
                bool seguir = true;
                
				/* for(int i = 0; i<list_size(lista_procesos_ejecutando); i++){
					t_unidad_ejecucion* unidad = (t_unidad_ejecucion*) list_get(lista_procesos_ejecutando, i);
					if(unidad->cpu->cpu_id == cpu_encargada->cpu_id){
						if(unidad->interrumpido == AEJECUTAR){
							seguir = false;
                            break;
						}
					}
				} */
                if(seguir){
                   // wait_mutex(&mutex_queue_ready);
                    t_pcb* proceso = list_remove(queue_ready, 0);
                    log_debug(logger_kernel, "Proceso removido por hilo principal pid <%d>", proceso->pid);
                    //signal_mutex(&mutex_queue_ready);
                    poner_en_ejecucion(proceso, cpu_encargada);
                } else {
                    //signal_sem(&cpu_libre); ///////////////////???????????????
                    //signal_sem(&desalojando);

                    //###############################
                    //signal_sem(&planificacion_principal); Revisar esto
                }
            } else {
                signal_sem(&proceso_ready);
                //esperar_cpu = true; //// cambio de posicion
            }
        }
        else{
            signal_sem(&cpu_libre);
        }
        signal_mutex(&mutex_pcb);
        signal_mutex(&mutex_queue_ready);
        signal_mutex(&mutex_lista_cpus);
        signal_mutex(&mutex_procesos_ejecutando);
        /*if(esperar_cpu){
            // int valorPP = 0;
		    // sem_getvalue(&check_desalojo, &valorPP);
            // if(valorPP > 0){
            signal_sem(&check_desalojo);
            //}
            wait_sem(&desalojo_revisado);
        }*/
    }
}

void *comprobar_desalojo(void *arg){
    bool desalojo_encontrado;
    while(1){
        desalojo_encontrado = false;
        wait_sem(&check_desalojo);
        wait_mutex(&desalojando);
        log_debug(logger_kernel, "Entro a revisar posible desalojo");
        wait_mutex(&mutex_queue_ready);
        wait_mutex(&mutex_procesos_ejecutando);
        wait_mutex(&mutex_pcb);
        list_sort(queue_ready, shortest_job_first);
        if(!list_is_empty(queue_ready)){
            t_pcb* proceso_prueba = list_get(queue_ready, 0);
            //signal_mutex(&mutex_queue_ready);
            log_debug(logger_kernel, "Compruebo procesos a desalojar");
            for(int i = 0; i<list_size(lista_procesos_ejecutando) && desalojo_encontrado == false; i++){
                t_unidad_ejecucion* proceso_ejecutando = list_get(lista_procesos_ejecutando, i);
                if(shortest_job_first_desalojo((void*) proceso_prueba, (void*) proceso_ejecutando->proceso) && proceso_ejecutando->interrumpido == EJECUTANDO){
                    //wait_mutex(&mutex_queue_ready);
                    list_remove_element(queue_ready, proceso_prueba);
                    //signal_mutex(&mutex_queue_ready);
                    log_debug(logger_kernel, "Proceso removido por hilo desalojo pid <%d>", proceso_prueba->pid);
                    t_paquete* paquete = crear_paquete(INTERRUPCION_CPU);
                    enviar_paquete(paquete, proceso_ejecutando->cpu->socket_interrupt);
                    log_debug(logger_kernel, "Envio interrupcion a cpu <%d> por proceso <%d>", 
                        proceso_ejecutando->cpu->cpu_id, proceso_ejecutando->proceso->pid);

                    proceso_ejecutando->interrumpido = INTERRUMPIDO;
                    
                    t_unidad_ejecucion* nueva = malloc(sizeof(t_unidad_ejecucion));
                    nueva->cpu = proceso_ejecutando->cpu;
                    nueva->proceso = proceso_prueba;
                    //nueva->tiempo_ejecutando = NULL;
                    nueva->interrumpido = AEJECUTAR;
                    list_add(lista_procesos_ejecutando, nueva);
                    desalojo_encontrado = true;
                }
            }
            log_debug(logger_kernel, "Salgo de comprobar procesos a desalojar");
            if(desalojo_encontrado == false){
                //int valorPP = 0;
				//sem_getvalue(&planificacion_principal, &valorPP);
                //if(planificacion_principal.__align <= 0){
                //if(valorPP <= 0){
                //signal_sem(&planificacion_principal);
                //}
                log_debug(logger_kernel, "No hubo desalojo y signal del proceso ready");
                signal_mutex(&desalojando);
                //signal_sem(&planificacion_principal);
            }
        } else {
            //signal_sem(&planificacion_principal);
            signal_mutex(&desalojando);
        }
        // int valorPP = 0;
		// sem_getvalue(&desalojo_revisado, &valorPP);
        // if(valorPP < 0){
        //signal_sem(&desalojo_revisado);
        //}
        signal_mutex(&mutex_pcb);
        signal_mutex(&mutex_procesos_ejecutando);
        signal_mutex(&mutex_queue_ready);
    }
        //signal_mutex(&mutex_cpu);   
}

bool shortest_job_first_desalojo(void* a, void* b){
    t_pcb* proceso_a = (t_pcb*) a;
    t_pcb* proceso_b = (t_pcb*) b;

    return proceso_a->estimacion_actual < proceso_b->estimacion_actual;
}

bool proceso_es_mas_chico(void* a, void* b){
    t_pcb* proceso_a = (t_pcb*) a;
    t_pcb* proceso_b = (t_pcb*) b;
    

    return proceso_a->tamanio_proceso <= proceso_b->tamanio_proceso;
}

bool hay_cpu_libre(t_cpu** cpu_encargada){
    t_cpu* cpu_libre = buscar_cpu_libre(lista_cpus);
    *cpu_encargada = cpu_libre;

    return cpu_libre != NULL;
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


void actualizar_estimaciones(){
    for(int i = 0; i<list_size(lista_procesos_ejecutando); i++){
        t_unidad_ejecucion* ejecucion = list_get(lista_procesos_ejecutando, i);
        t_pcb* proceso = ejecucion->proceso;
        if(ejecucion == NULL){
            log_debug(logger_kernel, "ERROR: ejecucion es NULL");
        }
        if(ejecucion->tiempo_ejecutando == NULL){
            log_debug(logger_kernel, "ERROR: tiempo_ejecutando es NULL");
        }
        if(ejecucion->interrumpido == EJECUTANDO){
            proceso->estimacion_actual -= temporal_gettime(ejecucion->tiempo_ejecutando);
        }
    }
}

void log_metricas_estado(t_pcb* proceso){
    long long tiempo_new = 0;
    long long tiempo_ready = 0;
    long long tiempo_exec = 0;
    long long tiempo_blocked = 0;
    long long tiempo_susp_ready = 0;
    long long tiempo_susp_blocked = 0;
    //log_debug(logger_kernel, "Cantidad de metricas finales de (<%d>): %d", proceso->pid, list_size(proceso->metricas_tiempo));
    for(int i = 0; i<list_size(proceso->metricas_tiempo);i++){
        t_metricas_estado_tiempo* metrica = list_get(proceso->metricas_tiempo, i);
        long long tiempo = calcular_tiempo(metrica);
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

    log_info(logger_kernel, "## %d - Metricas de estado: NEW %d %llu, READY %d %llu, EXEC %d %llu, BLOCKED %d %llu, SUSP_READY %d %llu, SUSP_BLOCKED %d %llu, EXIT 1", 
    proceso->pid, proceso->metricas_estado[0], tiempo_new, proceso->metricas_estado[1], tiempo_ready, proceso->metricas_estado[2], tiempo_exec, 
    proceso->metricas_estado[3], tiempo_blocked, proceso->metricas_estado[4], tiempo_susp_ready, proceso->metricas_estado[5], tiempo_susp_blocked);

}


long long calcular_tiempo(t_metricas_estado_tiempo* metrica){
    char* tiempo_inicio = metrica->tiempo_inicio;
    char* tiempo_fin = metrica->tiempo_fin;
    
    return obtener_diferencia_tiempo(tiempo_fin, tiempo_inicio);
}



long long obtener_diferencia_tiempo(char* tiempo_1, char* tiempo_2){
    int h1, m1, s1, ms1;
    int h2, m2, s2, ms2;

    sscanf(tiempo_1, "%d:%d:%d:%d", &h1, &m1, &s1, &ms1);
    sscanf(tiempo_2, "%d:%d:%d:%d", &h2, &m2, &s2, &ms2);

    long long total_ms1 = ((h1 * 3600) + (m1 * 60) + s1) * 1000 + ms1;
    long long total_ms2 = ((h2 * 3600) + (m2 * 60) + s2) * 1000 + ms2;

    long long diff_ms = total_ms1 - total_ms2;

    return diff_ms;
}


void *planificar_largo_plazo_FIFO(void* arg){
	while(1){
        wait_sem(&nuevo_proceso);
        //log_debug(logger_kernel, "Consumo semaforo nuevo_proceso, semaforo: %ld", nuevo_proceso.__align);
        wait_mutex(&mutex_queue_susp_ready);
        wait_mutex(&mutex_queue_new);
        wait_mutex(&mutex_queue_ready);
        wait_mutex(&mutex_lista_cpus);
        wait_mutex(&mutex_pcb);
        if(!list_is_empty(queue_susp_ready)){
            log_debug(logger_kernel, "entro a revisar suspendido_ready");
            //tiempo_en_io* proceso_a_desuspender = queue_peek(queue_susp_ready);
            //t_pcb* proceso = proceso_a_desuspender->pcb;
            t_pcb* proceso = list_get(queue_susp_ready, 0);
            signal_mutex(&mutex_queue_susp_ready);
            if(vuelta_swap(proceso)){

                //wait_mutex(&mutex_queue_susp_ready);
				list_remove_element(queue_susp_ready, proceso);
                //signal_mutex(&mutex_queue_susp_ready);

                // PONER EN READY
                poner_en_ready(proceso, false);
                //log_debug(logger_kernel, "Cola de ready:");
                //mostrar_lista(queue_ready);
                signal_mutex(&mutex_pcb);
                signal_mutex(&mutex_queue_susp_ready);
                signal_mutex(&mutex_queue_new);
                signal_mutex(&mutex_queue_ready);
                signal_mutex(&mutex_lista_cpus);
			}
            else{
                log_debug(logger_kernel, "No hay espacio en memoria para la vuelta de swap del proceso <%d>", proceso->pid);
                signal_sem(&nuevo_proceso); // el proceso nuevo sigue en NEW, hago signal de vuelta
                signal_mutex(&mutex_pcb);
                signal_mutex(&mutex_queue_susp_ready);
                signal_mutex(&mutex_queue_new);
                signal_mutex(&mutex_queue_ready);
                signal_mutex(&mutex_lista_cpus);
                wait_sem(&espacio_memoria); // espero que algun proceso finalice 
                //log_debug(logger_kernel, "Consumo semaforo espacio_memoria, semaforo: %ld", espacio_memoria.__align);
            }
        }
        else{
            //log_debug(logger_kernel, "entro a revisar new");
            //signal_mutex(&mutex_queue_susp_ready);
            if(!list_is_empty(queue_new)){
                t_pcb* proceso = list_get(queue_new, 0);
                //signal_mutex(&mutex_queue_new);
                if(espacio_en_memoria(proceso)){

                    //wait_mutex(&mutex_queue_new);
                    list_remove_element(queue_new, proceso);
                    //signal_mutex(&mutex_queue_new);

                    // PONER EN READY
                    //wait_mutex(&mutex_queue_ready);
                    //wait_mutex(&mutex_lista_cpus);
                    poner_en_ready(proceso, false);
                    //signal_mutex(&mutex_queue_ready);
                    //signal_mutex(&mutex_lista_cpus);
                    //log_debug(logger_kernel, "Cola de ready:");
                    //mostrar_lista(queue_ready);
                    signal_mutex(&mutex_pcb);
                    signal_mutex(&mutex_queue_susp_ready);
                    signal_mutex(&mutex_queue_new);
                    signal_mutex(&mutex_queue_ready);
                    signal_mutex(&mutex_lista_cpus);
                }
                else{
                    log_debug(logger_kernel, "No hay espacio en memoria para el proceso <%d>", proceso->pid);
                    signal_sem(&nuevo_proceso); // el proceso nuevo sigue en NEW, hago signal de vuelta
                    int valor = 0;
					sem_getvalue(&espacio_memoria, &valor);
                    //if(espacio_memoria.__align <= 0){
                    if(valor <= 0){
                        pthread_t hilo_espera_suspendido;
                        pthread_create(&hilo_espera_suspendido, NULL, (void*) comprobar_suspendido_ready, NULL);
                        pthread_detach(hilo_espera_suspendido);
                    }
                    signal_mutex(&mutex_pcb);
                    signal_mutex(&mutex_queue_susp_ready);
                    signal_mutex(&mutex_queue_new);
                    signal_mutex(&mutex_queue_ready);
                    signal_mutex(&mutex_lista_cpus);
                    wait_sem(&espacio_memoria); // espero que algun proceso finalice 
                    //log_debug(logger_kernel, "Consumo semaforo espacio_memoria, semaforo: %ld", espacio_memoria.__align);
                }
		    // } else {
            //     signal_mutex(&mutex_queue_new);
            }
        }
        
	}
}


void *planificar_largo_plazo_PMCP(void* arg){
    wait_mutex(&comprobar_memoria);

    /*pthread_t hilo_comprobar_nuevos;
    pthread_create(&hilo_comprobar_nuevos, NULL, (void*) comprobar_procesos_nuevos, arg);
    pthread_detach(hilo_comprobar_nuevos);*/
    
    pthread_t hilo_comprobar_suspendidos;
    pthread_create(&hilo_comprobar_suspendidos, NULL, (void*) comprobar_procesos_suspendidos, arg);
    pthread_detach(hilo_comprobar_suspendidos);

    pthread_t hilo_comprobar_espacio_memoria;
    pthread_create(&hilo_comprobar_espacio_memoria, NULL, (void*) comprobar_espacio, arg);
    pthread_detach(hilo_comprobar_espacio_memoria);

    while(1){
        wait_sem(&largo_plazo); //// agrego
        wait_mutex(&mutex_queue_susp_ready);
        log_debug(logger_kernel, "Compruebo suspendidos ready por espacio memoria");
        if(!list_is_empty(queue_susp_ready)){
            wait_mutex(&mutex_queue_ready);
            wait_mutex(&mutex_lista_cpus);
            wait_mutex(&mutex_pcb);
            list_sort(queue_susp_ready, proceso_es_mas_chico);
            t_pcb* proceso = list_get(queue_susp_ready, 0);
            if(vuelta_swap(proceso)){
                list_remove_element(queue_susp_ready, proceso);
                poner_en_ready(proceso, false);
                signal_mutex(&mutex_pcb);
                signal_mutex(&mutex_lista_cpus);
                signal_mutex(&mutex_queue_ready);
                signal_mutex(&mutex_queue_susp_ready);
            } else {
                signal_mutex(&mutex_pcb);
                signal_mutex(&mutex_lista_cpus);
                signal_mutex(&mutex_queue_ready);
                signal_mutex(&mutex_queue_susp_ready);
                signal_sem(&largo_plazo);
                wait_mutex(&mutex_sem_espacio_memoria);
                wait_sem(&espacio_memoria);
                signal_mutex(&mutex_sem_espacio_memoria);
            }
        }
        else{
            signal_mutex(&mutex_queue_susp_ready);
            wait_mutex(&mutex_queue_new);
            wait_mutex(&mutex_queue_ready);
            wait_mutex(&mutex_lista_cpus);
            wait_mutex(&mutex_pcb);
            log_debug(logger_kernel, "Compruebo new por espacio memoria");
            bool esperar = false;
            if(!list_is_empty(queue_new)){
                list_sort(queue_new, proceso_es_mas_chico);
                t_pcb *proceso = list_get(queue_new, 0);
                if(espacio_en_memoria(proceso)){
                    list_remove_element(queue_new, proceso);
                    poner_en_ready(proceso, false);
                } else {
                    esperar = true;
                }
                
            }
            signal_mutex(&mutex_pcb);
            signal_mutex(&mutex_queue_ready);
            signal_mutex(&mutex_lista_cpus);
            signal_mutex(&mutex_queue_new);
            if(esperar){
                signal_mutex(&comprobar_memoria);
                signal_sem(&check_susp_ready);
                signal_sem(&largo_plazo);
                wait_mutex(&mutex_susp_o_memoria);
            }
        }
    }
}
   
void* comprobar_ready(void* arg){
    wait_sem(&planificar);
    int valor = 0;
    sem_getvalue(&proceso_ready, &valor);
    //if(proceso_ready.__align <= 0){
    if(valor <= 0){
        signal_sem(&proceso_ready);
    }
    return NULL;
}

bool shortest_job_first(void* a, void* b){
    t_pcb* proceso_a = (t_pcb*) a;
    t_pcb* proceso_b = (t_pcb*) b;
    
    return proceso_a->estimacion_actual <= proceso_b->estimacion_actual;
}

void *comprobar_suspendido_ready(void* args){
    wait_sem(&nuevo_proceso_suspendido_ready);
    int valor = 0;
    sem_getvalue(&espacio_memoria, &valor);
    //if(espacio_memoria.__align < 0){
    if(valor < 0){
        signal_sem(&espacio_memoria);
    }
    return NULL;
}


void* comprobar_espacio(void* arg){
    while(1){
        wait_mutex(&comprobar_memoria);
        wait_mutex(&mutex_sem_espacio_memoria);
        wait_sem(&espacio_memoria);
        int valor = 0;
        sem_getvalue(&espacio_memoria, &valor);
        if(valor < 0){
            signal_sem(&espacio_memoria);
        }
        signal_mutex(&mutex_sem_espacio_memoria);

        signal_mutex(&mutex_susp_o_memoria);
    }
    
    return NULL;
}

void comprobar_procesos_suspendidos(void* arg){
    while(1){
        wait_sem(&check_susp_ready);
        //log_debug(logger_kernel, "Espero semaforo suspendido ready");
        wait_sem(&proceso_suspendido_ready);
        //log_debug(logger_kernel, "Consumo semaforo suspendido ready: %ld", proceso_suspendido_ready.__align);
        signal_mutex(&mutex_susp_o_memoria);
        //log_debug(logger_kernel, "Aviso que hay proceso en suspendido ready");
        /*wait_mutex(&mutex_queue_susp_ready);
        log_debug(logger_kernel, "Compruebo suspendidos ready por semaforo de suspendido");
        if(!queue_is_empty(queue_susp_ready)){
            t_pcb* proceso = queue_peek(queue_susp_ready);
            if(vuelta_swap(proceso)){
                queue_pop(queue_susp_ready);
                wait_mutex(&mutex_lista_cpus);
                wait_mutex(&mutex_queue_ready);
                poner_en_ready(proceso, false);
                signal_mutex(&mutex_queue_ready);
                signal_mutex(&mutex_lista_cpus);
            }
        }
        signal_mutex(&mutex_queue_susp_ready);*/
    }
}

void *comprobar_procesos_nuevos(void* arg){
    while(1){
        wait_sem(&nuevo_proceso);
        log_debug(logger_kernel, "Entro a comprobar procesos nuevos");
        wait_mutex(&mutex_queue_susp_ready);
        wait_mutex(&mutex_queue_new);
        wait_mutex(&mutex_queue_ready);
        wait_mutex(&mutex_lista_cpus);
        wait_mutex(&mutex_pcb);
        log_debug(logger_kernel, "Veo los procesos nuevos");
        if(!list_is_empty(queue_new)){
            list_sort(queue_new, proceso_es_mas_chico);
            t_pcb* proceso = list_get(queue_new, 0);
            if(list_is_empty(queue_susp_ready)){
                if(espacio_en_memoria(proceso)){
                    list_remove_element(queue_new, proceso);
                    poner_en_ready(proceso, false);
                }
            } 
            else {
                signal_sem(&proceso_suspendido_ready);
                log_debug(logger_kernel, "Signal del planificador_largo_plazo (comprobar procesos nuevos)");
            }
        }
        signal_mutex(&mutex_pcb);
        signal_mutex(&mutex_queue_new);
        signal_mutex(&mutex_queue_ready);
        signal_mutex(&mutex_lista_cpus);
        signal_mutex(&mutex_queue_susp_ready);
        
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
        liberar_conexion(conexion);
        return true;
    }
    else{
        liberar_conexion(conexion);
        return false;
    }

    //return 1;
}

bool vuelta_swap(t_pcb* proceso){
    //log_debug(logger_kernel, "Espero semaforo memoria swap");
    //wait_mutex(&mutex_memoria_swap);
    log_debug(logger_kernel, "Pregunto si hay espacio en memoria swap para el proceso <%d>", proceso->pid);
    int conexion = crear_conexion_memoria();
    t_paquete* paquete = crear_paquete(VUELTA_SWAP);

    agregar_a_paquete(paquete, &proceso->pid, sizeof(int));
    enviar_paquete(paquete, conexion);
    eliminar_paquete(paquete);

    int resultado;
    recv(conexion, &resultado, sizeof(int), 0);
    //signal_mutex(&mutex_memoria_swap);
    //log_debug(logger_kernel, "Libero semaforo memoria swap");
    if(resultado == OK){
        log_debug(logger_kernel, "Vuelta swap exitoso para PID: %d", proceso->pid);
    }
    else{
        log_debug(logger_kernel, "Vuelta swap erroneo para PID: %d", proceso->pid);
    }
    liberar_conexion(conexion);
    return resultado == OK;
}

void poner_en_ready(t_pcb* proceso, bool interrumpido){
    cambiar_estado(proceso, READY);
    log_debug(logger_kernel, "Pongo en ready al proceso <%d>", proceso->pid);
    list_add(queue_ready, proceso);
    mostrar_lista(queue_ready);
    signal_sem(&proceso_ready);
    
    t_cpu* cpu = NULL;
    if(strcmp(algoritmo_corto_plazo, "SRT") == 0){
        if(!interrumpido){
            signal_sem(&planificacion_principal);
            // if(hay_cpu_libre(&cpu)){
            //     log_debug(logger_kernel, "Signal del planificacion principal por proceso (<%d>)", proceso->pid);
            //     //signal_sem(&planificacion_principal);
            // } else{
            //     log_debug(logger_kernel, "Signal del check desalojo, por proceso (<%d>)", proceso->pid);
            //     //signal_sem(&check_desalojo);
            // }
        }
        /* int valor = -1;
        sem_getvalue(&check_desalojo, &valor); */
        if(!hay_cpu_libre(&cpu)){
            signal_sem(&check_desalojo);
        }
    }
}

void poner_en_ejecucion(t_pcb* proceso, t_cpu* cpu_encargada){
    t_paquete* paquete = crear_paquete(CONTEXTO_PROCESO);
    agregar_a_paquete(paquete, &proceso->pid, sizeof(int));
    agregar_a_paquete(paquete, &proceso->program_counter, sizeof(int));
        // enviar_paquete(paquete, cpu_libre->socket_dispatch);
    enviar_paquete(paquete, cpu_encargada->socket_dispatch);
    eliminar_paquete(paquete);
    log_debug(logger_kernel, "Envio proceso <%d> para ejecutar a cpu <%d>", proceso->pid, cpu_encargada->cpu_id);


    //wait_mutex(&mutex_procesos_ejecutando);
    //wait_mutex(&mutex_lista_cpus);
    if(!esta_ejecutando(proceso)){
        cpu_encargada->esta_libre = 0;
        t_unidad_ejecucion* nueva = malloc(sizeof(t_unidad_ejecucion));
        nueva->cpu = cpu_encargada;
        nueva->proceso = proceso;
        nueva->tiempo_ejecutando = temporal_create();
        nueva->interrumpido = EJECUTANDO;
        list_add(lista_procesos_ejecutando, nueva);
        for(int i = 0; i < list_size(lista_procesos_ejecutando); i++){
            t_unidad_ejecucion* actual = list_get(lista_procesos_ejecutando, i);
            log_debug(logger_kernel, "Proceso en ejecucion: <%d> - CPU: <%d> - Estado: %s", actual->proceso->pid, 
                actual->cpu->cpu_id, estado_ejecucion_to_string(actual->interrumpido));
        }
        cambiar_estado(proceso, EXEC);
    }
    //signal_mutex(&mutex_lista_cpus);
    //signal_mutex(&mutex_procesos_ejecutando);
}

bool esta_ejecutando(t_pcb* procesoAComprobar){
    for(int i =0; i<list_size(lista_procesos_ejecutando); i++){
        t_unidad_ejecucion* actual = list_get(lista_procesos_ejecutando, i);
        if(actual->proceso->pid == procesoAComprobar->pid){
            return true;
        }
    }
    return false;
}

void enviar_proceso_a_io(t_pcb* proceso, t_dispositivo_io* dispositivo, int io_tiempo){
    t_paquete* paquete = crear_paquete(PETICION_IO);
    agregar_a_paquete(paquete, &(proceso->pid), sizeof(int));
    agregar_a_paquete(paquete, &io_tiempo,sizeof(int));
    enviar_paquete(paquete, dispositivo->socket);

    log_info(logger_kernel, "## %d - Bloqueado por IO: %s", proceso->pid, dispositivo->nombre);
}

void comprobar_suspendido(t_pcb* proceso){
    usleep(tiempo_suspension * 1000); // tiempo_suspension en ms / 1000 = tiempo_suspension en us
    //log_debug(logger_kernel, "Suspendido espero semaforo memoria swap");
    //wait_mutex(&mutex_memoria_swap);
    wait_mutex(&mutex_queue_block);
    wait_mutex(&mutex_pcb);
    if(proceso->estado == BLOCKED){
        paquete_memoria_pid(proceso, SWAP);
        cambiar_estado(proceso, SUSP_BLOCKED);
        //log_debug(logger_kernel, "Suspendido libero semaforo memoria swap");
        //signal_mutex(&mutex_memoria_swap);
        log_debug(logger_kernel, "Proceso en suspendido, swap exitoso para PID: %d", proceso->pid);
        signal_sem(&espacio_memoria);
        signal_sem(&largo_plazo);
        /* signal_sem(&planificador_largo_plazo);
        log_debug(logger_kernel, "Signal del planificador_largo_plazo (%ld) por proceso suspendido pid <%d>", 
            planificador_largo_plazo.__align, proceso->pid); */
        //log_debug(logger_kernel, "HAY ESPACIO EN MEMORIA");
    }

    signal_mutex(&mutex_pcb);
    signal_mutex(&mutex_queue_block);
    //log_debug(logger_kernel, "Suspendido libero semaforo memoria swap");
    //signal_mutex(&mutex_memoria_swap);
    
}

void comprobar_cola_bloqueados(t_dispositivo_io* dispositivo){
    //wait_mutex(&mutex_queue_block);
    //wait_mutex(&mutex_lista_dispositivos_io);
    //t_queue* cola_io = obtener_esperando_io(dispositivo->id);
    //char id_dispositivo[12];
    //sprintf(id_dispositivo, "%d", dispositivo->id);
    //wait_mutex(&mutex_diccionario_esperando_io);
    //wait_mutex(&mutex_diccionario_ejecutando_io);
    t_queue* cola_io = obtener_esperando_io(dispositivo->nombre);
    if(!queue_is_empty(cola_io)){
        tiempo_en_io* proceso = queue_pop(cola_io);
        enviar_proceso_a_io(proceso->pcb, dispositivo, proceso->tiempo);
        list_add(obtener_ejecutando_io(dispositivo->nombre), proceso);
    }
    else{
        //signal_mutex(&mutex_queue_block);
        log_debug(logger_kernel, "## No hay procesos bloqueados en el dispositivo (%s, %d)", dispositivo->nombre, dispositivo->id);
        dispositivo->esta_libre = true;
    }
    //signal_mutex(&mutex_queue_block);
    //signal_mutex(&mutex_diccionario_esperando_io);
    //signal_mutex(&mutex_diccionario_ejecutando_io);
}
