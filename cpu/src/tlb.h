#ifndef TLB_H
#define TLB_H

#include <stdint.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/temporal.h>
#include <stdlib.h>

extern t_list* tlb;
extern int entradas_tlb;
extern char* algoritmo_tlb;


typedef struct {
    int pid;
    uint32_t pagina;
    uint32_t marco;
    uint64_t timestamp;
} t_entrada_tlb;

void inicializar_tlb(t_log* logger);
int buscar_en_tlb(int pid, uint32_t pagina, uint32_t* marco_resultado, t_log* logger);
void agregar_a_tlb(int pid, uint32_t pagina, uint32_t marco, t_log* logger);
void limpiar_tlb_por_pid(int pid);

#endif
