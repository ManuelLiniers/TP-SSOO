#include "tlb.h"
#include "utils/socket.h"

t_list* tlb;
int entradas_tlb;
char* algoritmo_tlb;

uint64_t get_timestamp() {
    return (uint64_t) time(NULL);
}

void inicializar_tlb(t_log* logger,  t_config* cpu_config) {
    entradas_tlb = config_get_int_value(cpu_config, "ENTRADAS_TLB");
    algoritmo_tlb = strdup(config_get_string_value(cpu_config, "REEMPLAZO_TLB"));
    if (entradas_tlb == 0) {  //PARA QUE LA TLB NO ESTÉ HABILITADA SI LAS ENTRADAS SON 0
    log_debug(logger, "La TLB está deshabilitada");
}   tlb = list_create();
    log_debug(logger, "TLB inicializada con %d entradas y reemplazo %s", entradas_tlb, algoritmo_tlb);
}

int buscar_en_tlb(int pid, uint32_t pagina, uint32_t* marco_resultado, t_log* logger) {
    if (entradas_tlb == 0) return 0;  //SI TIENE 0 ENTRADAS, NO SE HABILITA
    for (int i = 0; i < list_size(tlb); i++) {
        t_entrada_tlb* entrada = list_get(tlb, i);
        if (entrada->pid == pid && entrada->pagina == pagina) {
            *marco_resultado = entrada->marco;
            entrada->timestamp = get_timestamp();
            log_info(logger, "PID: %d - TLB HIT - Pagina: %d", pid, pagina);
            return 1;  //HUBO UN HIT Y TERMINA LA EJECUCUIÓN
        }
    }
    log_info(logger, "PID: %d - TLB MISS - Pagina: %d", pid, pagina);
    return 0;
}


void agregar_a_tlb(int pid, uint32_t pagina, uint32_t marco, t_log* logger) {
    if (list_size(tlb) >= entradas_tlb) {
        if (string_equals_ignore_case(algoritmo_tlb, "FIFO")) { //el elemento en la posición 0 es el más viejo (el primero que entró),seremueve y libera la memoria
            list_remove_and_destroy_element(tlb, 0, free);
        } else if (string_equals_ignore_case(algoritmo_tlb, "LRU")) {
            int index_de_entrada = 0;
            uint64_t min_timestamp = UINT64_MAX;
            for (int i = 0; i < list_size(tlb); i++) { //recorro toda la TLB buscando la entrada con el timestamp más viejo
                t_entrada_tlb* entrada = list_get(tlb, i);
                if (entrada->timestamp < min_timestamp) {  //ACTUALIZO LOS TIMESTAMP DE CADA ENTRADA PARA IDENTIFICAR LA MÁS VIEJA EN CASO DE USAR LRU DESPUÉS
                    min_timestamp = entrada->timestamp;
                    index_de_entrada = i;
                }
            }
            list_remove_and_destroy_element(tlb, index_de_entrada, free);
        }
    }

    t_entrada_tlb* nueva = malloc(sizeof(t_entrada_tlb));
    nueva->pid = pid;
    nueva->pagina = pagina;
    nueva->marco = marco;
    nueva->timestamp = get_timestamp();
    list_add(tlb, nueva);
}

void limpiar_tlb_por_pid(int pid) {
for (int i = 0; i < list_size(tlb);) {
        t_entrada_tlb* entrada = list_get(tlb, i);
        if (entrada->pid == pid) {
            list_remove_and_destroy_element(tlb, i, free);
        } else {
            i++;
        }
    }
}
