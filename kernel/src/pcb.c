#include <pcb.h>
#include <stdlib.h>

// Se crea un proceso y se pushea a new
void crear_proceso(t_list* instrucciones, int tamanio_proceso){
    t_pcb* pcb_nuevo = pcb_create();
    pcb_nuevo->instrucciones = instrucciones;
    pcb_nuevo->tamanio_proceso = tamanio_proceso;

    //wait(mutex_queue_new)
    queue_push(queue_new, pcb_nuevo);
    //signal(mutex_queue_new)
}

// Se crea un PCB con contador en 0 y en NEW
t_pcb* pcb_create() {
    t_pcb* pcb = malloc(sizeof(t_pcb));
    if (!pcb) return NULL;
    pcb->pid = pid_incremental;
    pcb->program_counter = 0;
    pcb->estado = NEW;

    pid_incremental++;

    // Ver como implementamos las listas de las metricas al crear la pcb

    inicializarMetricas(&pcb->metricas_estado);

    return pcb;
}

void inicializarMetricas(t_metricas_cant** metricas){
    t_metricas_cant* sig = NULL;
    for(int i = 4 ; i>=0 ; i--){
        t_metricas_cant* aux = malloc(sizeof(t_metricas_cant));
        t_estado e = posibles_estados[i];
        aux->estado = e;
        aux->cant = 0;
        aux->sig = sig;
        sig = aux;
    }
    *metricas = sig;
}

// Destruyo el PCB y libero lista de inst
void pcb_destroy(void* pcb_void) {
    t_pcb* pcb = (t_pcb*)pcb_void;
    if (!pcb) return;
    // Asumimos que cada instrucción fue duplicada con malloc/string_new
    list_destroy_and_destroy_elements(pcb->instrucciones, free);
    free(pcb);
}
