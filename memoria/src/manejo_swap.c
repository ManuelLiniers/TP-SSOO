#include "../include/manejo_swap.h"

void enviar_a_swap(int pid) {
    t_proceso* proceso = obtener_proceso_por_id(pid);

    FILE* archivo = fopen(PATH_SWAPFILE, "w");
    if (!archivo) {
        log_error(memoria_logger, "No existe el swapfile");
        return;
    }

    int paginas = proceso->paginas;
    t_list* lista_desplazamiento = obtener_espacios_swap(pid,  paginas);

    escribir_marcos_en_archivo(archivo, proceso->tabla_paginas_raiz, &paginas);

    list_destroy(lista_desplazamiento);
    fclose(archivo);
}

t_list* obtener_espacios_swap(int pid, int cantidad_paginas){
    t_list* lista_desplazamiento = list_create();
    for(int i = 0; i < cantidad_paginas; i++){
        int espacio_libre = primer_hueco_libre();
        
        int* desplazamiento = malloc(sizeof(int));
        int* pid_lista = malloc(sizeof(int));
        *pid_lista = pid;

        if(espacio_libre != -1){
            *desplazamiento = espacio_libre * TAM_PAGINA;
            list_replace(lista_swap, espacio_libre, pid_lista);
        } else {
            *desplazamiento = list_size(lista_swap) * TAM_PAGINA;
            list_add(lista_swap, pid_lista);
        }
            list_add(lista_desplazamiento, desplazamiento);
    }
    return lista_desplazamiento;
}

int primer_hueco_libre() {
    for (int i = 0; i < list_size(lista_swap); i++)
        if (*(int*)list_get(lista_swap, i) == -1)
            return i;
    return -1;
}

void escribir_marcos_en_archivo_con_desplazamiento(FILE* archivo, t_tabla_nivel* tabla, int* paginas_restantes, t_list* lista_desplazamientos){
    for (int i = 0; i < ENTRADAS_POR_TABLA && *paginas_restantes > 0; i++) {

        if (tabla->nivel == CANTIDAD_NIVELES) {
            t_pagina* pagina = (t_pagina*) list_get(tabla->entradas, i);
            int marco = pagina->marco_asignado; 

            if (marco != -1) {
                int* desplazamiento = list_get(lista_desplazamientos, 0);
                poner_marco_en_archivo_con_desplazamiento(archivo, marco, *desplazamiento);
                list_remove(lista_desplazamientos, 0);
                liberar_marco(marco);
                pagina->marco_asignado = -1;
            }
        } else {
            t_tabla_nivel* subtabla = (t_tabla_nivel*) list_get(tabla->entradas, i);
            asignar_marcos_a_tabla(subtabla, paginas_restantes);
        }
    }
}

void poner_marco_en_archivo_con_desplazamiento(FILE* archivo, int marco, int desplazamiento){

    if (archivo == NULL || marco < 0) {
        log_error(memoria_logger, "Parámetros inválidos");
        return;
    }

    void* buffer_para_escritura = malloc(TAM_PAGINA);

    pthread_mutex_lock(&mutex_espacio_usuario);
    memcpy(buffer_para_escritura, espacio_usuario + marco * TAM_PAGINA, TAM_PAGINA);    
    pthread_mutex_unlock(&mutex_espacio_usuario);

    fseek(archivo, desplazamiento, SEEK_SET);
    fwrite(buffer_para_escritura, 1, TAM_PAGINA, archivo);
}