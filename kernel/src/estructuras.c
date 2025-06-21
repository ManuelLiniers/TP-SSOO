#include "estructuras.h"
#include <stdlib.h>

int pid_incremental = 0;
int id_io_incremental = 0;
t_list* lista_dispositivos_io;
t_list* lista_cpus;
t_list* lista_procesos_ejecutando;
int estimacion_inicial;
double estimador_alfa;

// Definición de las colas globales
t_queue* queue_new;
t_list* queue_new_PMCP;
t_queue* queue_ready;
t_list* queue_ready_SJF;
t_list* queue_block;
t_queue* queue_exit;

void scheduler_init(void) {
    queue_new  = queue_create();
    queue_new_PMCP = list_create();
    queue_ready = queue_create();
    queue_ready_SJF = list_create();
    queue_block = list_create();
    queue_exit = queue_create();
    lista_procesos_ejecutando = list_create();

}

void scheduler_destroy(void) {
    queue_destroy_and_destroy_elements(queue_new, pcb_destroy);
    queue_destroy_and_destroy_elements(queue_ready, pcb_destroy);
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
    if (!pcb) return NULL;
    pcb->pid = pid_incremental;
    pcb->program_counter = 0;
    pcb->estado = NEW;
    int registros[4] = {0, 0, 0, 0};
    memcpy(pcb->registros, registros, sizeof(int[4]));

    pid_incremental++;

    int metricas_estado[7] = {0, 0, 0, 0, 0, 0, 0};
    memcpy(pcb->metricas_estado, metricas_estado, sizeof(int[5]));
    pcb->metricas_tiempo = list_create();
    pcb->cpu_encargada = NULL;

    /* tengo una idea para implementar las metricas que capaz es mas simple
    no lo pense tanto en codigo pero capaz que sea un vector con 5 espacios
    donde cada estado equivalga a cada espacio, hacemos una funcion que
    devuelva la posicion segun el estado que pidas para poder acceder
    ejemplo: pcb->metricas[id_estado(NEW)], id_estado(NEW) devolveria 0 
    */

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

t_dispositivo_io* buscar_io(int id_io){
    for(int i = 0; i<list_size(lista_dispositivos_io); i++){
        t_dispositivo_io* actual = (t_dispositivo_io*) list_get(lista_dispositivos_io, i);
        if(actual->id == id_io){
            return actual;
        }
    }
    return NULL;
}

t_cpu* buscar_cpu_libre(t_list* lista_cpus){
    for(int i = 0; i<list_size(lista_cpus); i++){
        t_cpu* actual = list_get(lista_cpus, i);
        if(actual->esta_libre){
            return actual;
        }
    }
    log_error(logger_kernel, "No hay CPUs libres");
    return NULL;
}

t_pcb* buscar_proceso_pid(uint32_t pid){
    for(int i=0; i<list_size(lista_procesos_ejecutando); i++){
        t_pcb* actual = list_get(lista_procesos_ejecutando, i);
        if(actual->pid == (uint32_t)pid){
            //list_remove(lista_procesos_ejecutando, i); 
            // no eliminar aca porque hay respuestas que no necesariamente sacan al proceso de ejecucion, ejemplo INIT_PROC
            return actual;
        }
    }
    return NULL;
}

void sacar_proceso_ejecucion(t_pcb* proceso){
    list_remove_element(lista_procesos_ejecutando, proceso);
    signal_sem(&espacio_memoria);
}

void cambiarEstado(t_pcb* proceso, t_estado estado){
    t_metricas_estado_tiempo* metrica_anterior = obtener_ultima_metrica(proceso);

    if(metrica_anterior != NULL){
        metrica_anterior->tiempo_fin = (double)time(NULL); // o se puede cambiar por time()
        if(metrica_anterior->estado == EXEC && estado == BLOCKED ){
            t_list* estados_exec = obtener_exec(proceso);
            proceso->rafaga_real = calcular_rafaga(estados_exec);
            calcular_estimacion(proceso);
        }
    }

    proceso->estado = estado;
    proceso->metricas_estado[id_estado(estado)]++;

    if(estado != EXIT){
        t_metricas_estado_tiempo* metrica = malloc(sizeof(t_metricas_estado_tiempo));
        metrica->estado = estado;
        metrica->tiempo_inicio = (double)time(NULL);
        list_add(proceso->metricas_tiempo, metrica);
    }

    if(metrica_anterior == NULL){
        log_info(logger_kernel, "## (<%d>) Pasa al estado <%s>", proceso->pid, estado_to_string(estado));
    }
    else{
        log_info(logger_kernel, "## (<%d>) Pasa del estado <%s> al estado <%s>", proceso->pid, estado_to_string(metrica_anterior->estado), estado_to_string(estado));
    }
}

void calcular_estimacion(t_pcb* proceso){
    proceso->estimacion_actual = proceso->rafaga_real * estimador_alfa + proceso->estimacion_anterior * (1-estimador_alfa);
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