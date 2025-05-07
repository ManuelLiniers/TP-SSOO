#include <pcb.h>
#include <stdlib.h>

// Se crea un PCB con contador en 0 y en NEW
t_pcb* pcb_create(int pid, t_list* instrucciones) {
    t_pcb* pcb = malloc(sizeof(t_pcb));
    if (!pcb) return NULL;
    pcb->pid = pid;
    pcb->program_counter = 0;
    pcb->instrucciones = instrucciones;
    pcb->estado = NEW;

    // Ver como implementamos las listas de las metricas al crear la pcb
    
    return pcb;
}

// Destruyo el PCB y libero lista de inst
void pcb_destroy(void* pcb_void) {
    t_pcb* pcb = (t_pcb*)pcb_void;
    if (!pcb) return;
    // Asumimos que cada instrucción fue duplicada con malloc/string_new
    list_destroy_and_destroy_elements(pcb->instrucciones, free);
    free(pcb);
}
