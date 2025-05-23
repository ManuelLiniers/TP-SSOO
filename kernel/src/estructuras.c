#include "estructuras.h"
#include <stdlib.h>

void iniciar_dispositivos_io(){
    lista_dispositivos_io = list_create();
}

void iniciar_cpus(){
    lista_cpus = list_create();
}

// Se crea un proceso y se pushea a new
void crear_proceso(char* instrucciones, char* tamanio_proceso){
    t_pcb* pcb_nuevo = pcb_create();
    pcb_nuevo->instrucciones = instrucciones;
    pcb_nuevo->tamanio_proceso = atoi(tamanio_proceso);
    
    wait_mutex(&mutex_queue_new);
    queue_push(queue_new, pcb_nuevo);
    signal_mutex(&mutex_queue_new);
    signal_sem(&nuevo_proceso);
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
