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
