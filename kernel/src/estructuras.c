#include "estructuras.h"
#include <stdlib.h>

int pid_incremental = 0;
int id_io_incremental = 0;
t_list* lista_dispositivos_io;
t_list* lista_cpus;
t_list* lista_procesos_ejecutando;

// Definición de las colas globales
t_queue* queue_new;
t_list* queue_new_PMCP;
t_queue* queue_ready;
t_list* queue_block;
t_queue* queue_exit;

void scheduler_init(void) {
    queue_new  = queue_create();
    queue_new_PMCP = list_create();
    queue_ready = queue_create();
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

    int metricas_estado[5] = {0, 0, 0, 0, 0};
    memcpy(pcb->metricas_estado, metricas_estado, sizeof(int[5]));
    pcb->metricas_tiempo = list_create();

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
    case EXIT:
        return 4;
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
    log_error(logger_kernel, "No se hay CPUs libres");
    return NULL;
}

t_pcb* buscar_proceso_pid(uint32_t pid){
    for(int i=0; i<list_size(lista_procesos_ejecutando); i++){
        t_pcb* actual = list_get(lista_procesos_ejecutando, i);
        if(actual->pid == (uint32_t)pid){
            list_remove(lista_procesos_ejecutando, i);
            return actual;
        }
    }
    return NULL;
}

void cambiarEstado(t_pcb* proceso, t_estado estado){

    t_metricas_estado_tiempo* metrica_anterior = obtener_ultima_metrica(proceso);
    if(metrica_anterior != NULL){
        metrica_anterior->tiempo_fin = clock(); // o se puede cambiar por time()
        if(metrica_anterior->estado == EXEC && estado == BLOCKED){
            proceso->rafaga_real = calcular_rafaga(proceso->metricas_tiempo);
        }
    }

    proceso->estado = estado;
    proceso->metricas_estado[id_estado(estado)]++;


    t_metricas_estado_tiempo* metrica = malloc(sizeof(t_metricas_estado_tiempo));
    metrica->estado = estado;
    metrica->tiempo_inicio = clock();

    list_add(proceso->metricas_tiempo, metrica);
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

int calcular_rafaga(t_list* metricas_tiempo){
    t_metricas_estado_tiempo* ultima_metrica = list_get(metricas_tiempo, sizeof(metricas_tiempo)-1); 
    t_metricas_estado_tiempo* metrica = list_get(metricas_tiempo, sizeof(metricas_tiempo)-3);
    if(metrica->estado == EXEC){
        return ultima_metrica->tiempo_fin - metrica->tiempo_inicio - (ultima_metrica->tiempo_inicio - metrica->tiempo_fin);
    }
    else{
        return ultima_metrica->tiempo_fin - ultima_metrica->tiempo_inicio;
    }
}

void comprobar_cola_bloqueados(int io_id){

    t_queue* cola_io = obtener_cola_io(io_id);

    if(!queue_is_empty(cola_io)){
        tiempo_en_io *proceso = queue_peek(cola_io);

        enviar_proceso_a_io(proceso->pcb, io_id, proceso->tiempo);
    }
}

t_queue* obtener_cola_io(int io_id){
    return list_get(queue_block, io_id);
}