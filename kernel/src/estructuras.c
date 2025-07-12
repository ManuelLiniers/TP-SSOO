#include "estructuras.h"
#include <stdlib.h>

int pid_incremental = 0;
int id_io_incremental = 0;
t_list* lista_dispositivos_io;
t_list* lista_cpus;
t_list* lista_procesos_ejecutando;
int estimacion_inicial;
int tiempo_suspension;
double estimador_alfa;
char* log_level;

// Definición de las colas globales
t_list* queue_new;
t_list* queue_ready;
t_list* queue_ready_SJF;
t_list* queue_block;
t_queue* queue_susp_ready;
t_queue* queue_exit;

void agregar_a_lista(void* cola_ready, t_pcb* proceso){
    list_add( (t_list*) cola_ready, proceso); 
}

void agregar_a_cola(void* cola_ready, t_pcb* proceso){
    queue_push((t_queue*) cola_ready, proceso);
}

void scheduler_init(void) {
    queue_new = list_create();
    queue_ready = list_create();
    queue_block = list_create();
    queue_susp_ready = queue_create();
    queue_exit = queue_create();
    lista_procesos_ejecutando = list_create();

}

void scheduler_destroy(void) {
    list_destroy_and_destroy_elements(queue_new, pcb_destroy);
    list_destroy_and_destroy_elements(queue_ready, pcb_destroy);
    list_destroy_and_destroy_elements(queue_block, pcb_destroy);
    queue_destroy_and_destroy_elements(queue_exit, pcb_destroy);
}

void iniciar_dispositivos_io(){
    lista_dispositivos_io = list_create();
}

void iniciar_cpus(){
    lista_cpus = list_create();
}

// Se crea un PCB con contador en 0 y en NEW
t_pcb* pcb_create() {
    t_pcb* pcb = malloc(sizeof(t_pcb));
    if (pcb == NULL){ 
        return NULL;
    };
    pcb->pid = pid_incremental;
    pcb->program_counter = 0;
    pcb->estado = NEW;

    pid_incremental++;

    int metricas_estado[7] = {0, 0, 0, 0, 0, 0, 0};
    memcpy(pcb->metricas_estado, metricas_estado, sizeof(int[5]));
    pcb->metricas_tiempo = list_create();

    return pcb;
}


int id_estado(t_estado estado){
    switch (estado)
    {
    case NEW:
        return 0;
        break;
    case READY:
        return 1;
        break;
    case EXEC:
        return 2;
        break;
    case BLOCKED:
        return 3;
        break;
    case SUSP_READY:
        return 4;
        break;
    case SUSP_BLOCKED:
        return 5;
        break;
    case EXIT:
        return 6;
        break;
    default:
        return -1;
        break;
    }
}


// Destruyo el PCB y libero lista de inst
void pcb_destroy(void* pcb_void) {
    t_pcb* pcb = (t_pcb*)pcb_void;
    if (!pcb) return;
    // Asumimos que cada instrucción fue duplicada con malloc/string_new
    //list_destroy_and_destroy_elements(pcb->instrucciones, free); 
    free(pcb);
}

t_dispositivo_io* buscar_io(char* nombre_io){
    for(int i = 0; i<list_size(lista_dispositivos_io); i++){
        t_dispositivo_io* actual = list_get(lista_dispositivos_io, i);
        if(strcmp(nombre_io, actual->nombre) == 0){
            return actual;
        }
    }
    return NULL;
}

t_dispositivo_io* buscar_io_libre(char* nombre_io){
    log_debug(logger_kernel, "Dispositivo a buscar: %s", nombre_io);
    for(int i = 0; i<list_size(lista_dispositivos_io); i++){
        t_dispositivo_io* actual =list_get(lista_dispositivos_io, i);
        wait_mutex(&mutex_queue_block);
        if(strcmp(nombre_io, actual->nombre) == 0 && queue_is_empty(obtener_cola_io(actual->id))){
            signal_mutex(&mutex_queue_block);
            return actual;
        }
        signal_mutex(&mutex_queue_block);
    }
    return NULL;
}

t_dispositivo_io* buscar_io_menos_ocupada(char* nombre_io){
    t_dispositivo_io* dispositivo = buscar_io(nombre_io);
    for(int i=0; i<list_size(lista_dispositivos_io); i++){
        if(dispositivo != NULL){
            log_debug(logger_kernel, "Buscando IO libre, dispositivo actual: %s", dispositivo->nombre);
        }
        else{
            log_debug(logger_kernel, "Buscando IO menos ocupada, dispositivo actual: no hay dispositivo");
        }
        t_dispositivo_io* siguiente = (t_dispositivo_io*) list_get(lista_dispositivos_io, i);
        wait_mutex(&mutex_queue_block);
        if(siguiente->nombre == nombre_io && queue_size(obtener_cola_io(siguiente->id)) < queue_size(obtener_cola_io(dispositivo->id))){
            dispositivo = siguiente;
        }
        signal_mutex(&mutex_queue_block);
    }
    return dispositivo;
}

t_cpu* buscar_cpu_libre(t_list* lista_cpus){
    for(int i = 0; i<list_size(lista_cpus); i++){
        t_cpu* actual = list_get(lista_cpus, i);
        mostrar_cpu(actual);
        if(actual->esta_libre){
            return actual;
        }
    }
    log_error(logger_kernel, "No hay CPUs libres");
    return NULL;
}

void liberar_cpu(t_cpu* cpu_encargada){
    for(int i=0; i<list_size(lista_cpus); i++){
        t_cpu* actual = list_get(lista_cpus, i);
        if(actual->cpu_id == cpu_encargada->cpu_id){
            actual->esta_libre = 1;
        }
    }
}

t_pcb* buscar_proceso_pid(uint32_t pid){
    for(int i=0; i<list_size(lista_procesos_ejecutando); i++){
        t_unidad_ejecucion* actual = list_get(lista_procesos_ejecutando, i);
        if(actual->proceso->pid == (uint32_t)pid){
            return actual->proceso;
        }
    }
    return NULL;
}

void sacar_proceso_ejecucion(t_pcb* proceso){
    for(int i = 0; i<list_size(lista_procesos_ejecutando); i++){
        t_unidad_ejecucion* actual = list_get(lista_procesos_ejecutando, i);
        if(actual->proceso->pid == proceso->pid){
            list_remove_element(lista_procesos_ejecutando, actual);
            temporal_destroy(actual->tiempo_ejecutando);
        }
    }
    signal_sem(&cpu_libre);
}

void cambiar_estado(t_pcb* proceso, t_estado estado){
    t_metricas_estado_tiempo* metrica_anterior = obtener_ultima_metrica(proceso);

    if(metrica_anterior != NULL){
        metrica_anterior->tiempo_fin = temporal_get_string_time("%H:%M:%S:%MS");
        log_info(logger_kernel, "Tiempo de fin en estado %s: %s", estado_to_string(metrica_anterior->estado), metrica_anterior->tiempo_fin); 
        switch (metrica_anterior->estado)
        {
        case EXEC:
            if(estado == BLOCKED){
                temporal_stop(proceso->rafaga_real);
                calcular_estimacion(proceso);
                temporal_destroy(proceso->rafaga_real);
            }
            if(estado == READY){
                temporal_stop(proceso->rafaga_real);
            }
            break;
        case READY:
            if(no_fue_desalojado(proceso)){
                proceso->rafaga_real = temporal_create();
            }
            else{
                temporal_resume(proceso->rafaga_real);
            }
            break;
        default:
            break;
        }
    }

    proceso->estado = estado;
    proceso->metricas_estado[id_estado(estado)]++;

    if(estado != EXIT){
        t_metricas_estado_tiempo* metrica = malloc(sizeof(t_metricas_estado_tiempo));
        metrica->estado = estado;
        metrica->tiempo_inicio = temporal_get_string_time("%H:%M:%S:%MS");
        log_info(logger_kernel, "Tiempo de inicio en estado %s: %s", estado_to_string(estado), metrica->tiempo_inicio);
        list_add(proceso->metricas_tiempo, metrica);
    }

    if(metrica_anterior == NULL){
        log_info(logger_kernel, "## (<%d>) Pasa al estado <%s>", proceso->pid, estado_to_string(estado));
    }
    else{
        log_info(logger_kernel, "## (<%d>) Pasa del estado <%s> al estado <%s>", proceso->pid, estado_to_string(metrica_anterior->estado), estado_to_string(estado));
        log_info(logger_kernel, "Tiempo en estado %s: %ld", estado_to_string(metrica_anterior->estado),
        obtener_diferencia_tiempo(metrica_anterior->tiempo_inicio, metrica_anterior->tiempo_fin));
    }
}

bool no_fue_desalojado(t_pcb* proceso){
    t_metricas_estado_tiempo* metrica = list_get(proceso->metricas_tiempo, list_size(proceso->metricas_tiempo)-2);
    return metrica->estado != EXEC;
}

void calcular_estimacion(t_pcb* proceso){
    proceso->estimacion_actual = temporal_gettime(proceso->rafaga_real) * estimador_alfa + proceso->estimacion_anterior * (1-estimador_alfa);
    proceso->estimacion_anterior = proceso->estimacion_actual;
}

char* estado_to_string(t_estado estado){
    switch (estado)
    {
    case NEW:
        return "NEW";
        break;
    case READY:
        return "READY";
        break;
    case EXEC:
        return "EXEC";
        break;
    case BLOCKED:
        return "BLOCKED";
        break;
    case SUSP_READY:
        return "SUSP_READY";
        break;
    case SUSP_BLOCKED:
        return "SUSP_BLOCKED";
        break;
    case EXIT:
        return "EXIT";
        break;
    default:
        return NULL;
        break;
    }
}

t_list* obtener_exec(t_pcb* proceso){
    t_list* list_exec = list_create();
    int i = list_size(proceso->metricas_tiempo)-1;
    t_metricas_estado_tiempo* metrica = list_get(proceso->metricas_tiempo, i); 
    for(; metrica->estado != BLOCKED; i--){
        if(metrica->estado == EXEC){
            list_add(list_exec, metrica);
        }
        metrica = list_get(proceso->metricas_tiempo, i);
    }
    return list_exec;
}

t_metricas_estado_tiempo* obtener_ultima_metrica(t_pcb* proceso){
    if(list_size(proceso->metricas_tiempo) == 0)
    {
        return NULL;
    }
    else
    {
        t_metricas_estado_tiempo* ultimo_elemento = list_get(proceso->metricas_tiempo, (list_size(proceso->metricas_tiempo)-1));
        return ultimo_elemento;
    }
}

long calcular_rafaga(t_list* estados_exec){
    long tiempo_total;
    for(int i = 0; i<list_size(estados_exec);i++){
        t_metricas_estado_tiempo* metrica = list_get(estados_exec, i);
        tiempo_total += (metrica->tiempo_fin - metrica->tiempo_inicio);
    }
    return tiempo_total;
}

t_queue* obtener_cola_io(int io_id){
    return list_get(queue_block, io_id);
}

void mostrar_cola(t_queue** cola){
    if(queue_is_empty(*cola)){
        log_debug(logger_kernel, "La cola esta vacia");
    }
    else{
        t_queue* aux = queue_create();
        for(int i = 0; i<queue_size(*cola); i++){
            t_pcb* proceso = queue_pop(*cola);
            queue_push(aux, proceso);
            log_debug(logger_kernel, "PID: (<%d>)", proceso->pid);
        }
        *cola=aux;
        log_debug(logger_kernel, "------");
    }
}

void mostrar_cola_io(t_queue** cola){
    if(queue_is_empty(*cola)){
        log_debug(logger_kernel, "La cola esta vacia");
    }
    else{
        t_queue* aux = queue_create();
        for(int i = 0; i<queue_size(*cola); i++){
            tiempo_en_io* proceso = queue_pop(*cola);
            if(proceso == NULL){
                log_debug(logger_kernel, "Proceso en cola io nulo");
                return;
            }
            queue_push(aux, proceso);
            log_debug(logger_kernel, "PID: (<%d>), ESTADO: %s", proceso->pcb->pid, estado_to_string(proceso->pcb->estado));
        }
        *cola=aux;
        log_debug(logger_kernel, "------");
    }
}

void mostrar_lista(t_list* lista){
    if(list_is_empty(lista)){
        log_debug(logger_kernel, "La lista esta vacia");
    }
    else{
        for(int i = 0; i<list_size(lista); i++){
            t_pcb* proceso = list_get(lista, i);
            log_debug(logger_kernel, "PID: (<%d>)", proceso->pid);
        }
        log_debug(logger_kernel, "------");
    }
}

void mostrar_cpus(){
    log_debug(logger_kernel, "Lista de CPUs:");
    if(list_is_empty(lista_cpus)){
        log_debug(logger_kernel, "La lista esta vacia");
    }
    else{
        for(int i = 0; i<list_size(lista_cpus); i++){
            t_cpu* cpu = list_get(lista_cpus, i);
            mostrar_cpu(cpu);
        }
        log_debug(logger_kernel, "------");
    }
}

void mostrar_cpu(t_cpu* cpu){
    log_debug(logger_kernel, "ID: (<%d>)", cpu->cpu_id);
    log_debug(logger_kernel, "Estado: %d", cpu->esta_libre);
}