#include "../include/manejo_swap.h"

int enviar_a_swap(int pid) {
    t_proceso* proceso = obtener_proceso_por_id(pid, procesos_memoria);

    int paginas = proceso->paginas;
    bool paginas_extra = false;
    t_list* lista_desplazamiento = obtener_espacios_swap(pid,  paginas, &paginas_extra);

    FILE* archivo = fopen(PATH_SWAPFILE, "r+b");
    if(!archivo){
        archivo = fopen(PATH_SWAPFILE, "w+b"); // no deberia entrar porque ya se crea en inicializar memoria
    }
    if(!archivo){
        log_error(memoria_logger, "No se pudo abrir el swapfile");
        return -1;
    }

    if(paginas_extra){
        int fd = fileno(archivo);
        ftruncate(fd, list_size(lista_swap) * TAM_PAGINA);
    }

    escribir_marcos_en_archivo_con_desplazamiento(archivo, proceso->tabla_paginas_raiz, &paginas, lista_desplazamiento);

    proceso->metricas->bajadas_swap++;
    list_destroy_and_destroy_elements(lista_desplazamiento, free);
    fclose(archivo);

    list_add(procesos_swap, proceso);
    list_remove_element(procesos_memoria, proceso);

    return 0;
}

t_list* obtener_espacios_swap(int pid, int cantidad_paginas, bool* paginas_extra){
    t_list* lista_desplazamiento = list_create();
    for(int i = 0; i < cantidad_paginas; i++){
        int espacio_libre = primer_hueco_libre();
        
        int* desplazamiento = calloc(1, sizeof(int));
        int* pid_lista = malloc(sizeof(int));
        *pid_lista = pid;

        if(espacio_libre != -1){
            *desplazamiento = espacio_libre * TAM_PAGINA;
            int* anterior = list_replace(lista_swap, espacio_libre, pid_lista);
            free(anterior);
        } else {
            *desplazamiento = list_size(lista_swap) * TAM_PAGINA;
            list_add(lista_swap, pid_lista);
            (*paginas_extra) = true;
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
                (*paginas_restantes)--;
            }
        } else {
            t_tabla_nivel* subtabla = (t_tabla_nivel*) list_get(tabla->entradas, i);
            escribir_marcos_en_archivo_con_desplazamiento(archivo, subtabla, paginas_restantes, lista_desplazamientos);
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

    free(buffer_para_escritura);
}

int sacar_de_swap(int pid){
    t_proceso* proceso = obtener_proceso_por_id(pid, procesos_swap);

    if (!proceso) {
        log_error(memoria_logger, "No se encontró el proceso con PID %d en SWAP", pid);
        return -1;
    }

    int paginas = proceso->paginas;

    t_list* lista_desplazamiento = obtener_desplazamientos_swap(pid,  paginas);

    FILE* archivo = fopen(PATH_SWAPFILE, "r+b");
    if(!archivo){
        archivo = fopen(PATH_SWAPFILE, "w+b");
    }
    if(!archivo){
        log_error(memoria_logger, "No se pudo abrir el swapfile");
        return -1;
    }

    escribir_paginas_en_marcos(archivo, proceso->tabla_paginas_raiz, &paginas, lista_desplazamiento);

    list_destroy_and_destroy_elements(lista_desplazamiento, free);
    fclose(archivo);

    list_add(procesos_memoria, proceso);
    list_remove_element(procesos_swap, proceso);
    return 0;
}

t_list* obtener_desplazamientos_swap(int pid, int cantidad_paginas){
    t_list* lista_desplazamiento = list_create();
    for(int i = 0; i < cantidad_paginas; i++){
        int pag_archivo = primer_pag_archivo(pid);

        if(pag_archivo == -1){
            log_error(memoria_logger, "No se encontró página en swap para PID %d", pid);
            list_destroy_and_destroy_elements(lista_desplazamiento, free);
            return NULL;
        }
        
        int* desplazamiento = malloc(sizeof(int));
        int* pid_lista = malloc(sizeof(int));
        *pid_lista = -1;

        *desplazamiento = pag_archivo * TAM_PAGINA;
        int* anterior = list_replace(lista_swap, pag_archivo, pid_lista);
        free(anterior);
    
        list_add(lista_desplazamiento, desplazamiento);
    }
    return lista_desplazamiento;
}

int primer_pag_archivo(int pid) {
    for (int i = 0; i < list_size(lista_swap); i++)
        if (*(int*)list_get(lista_swap, i) == pid)
            return i;
    return -1;
}

void escribir_paginas_en_marcos(FILE* archivo, t_tabla_nivel* tabla, int* paginas_restantes, t_list* lista_desplazamientos){
    for (int i = 0; i < ENTRADAS_POR_TABLA && *paginas_restantes > 0; i++) {

        if (tabla->nivel == CANTIDAD_NIVELES) {
            t_pagina* pagina = (t_pagina*) list_get(tabla->entradas, i);
            int marco = obtener_marco_libre(); 

            if (marco != -1) {
                int* desplazamiento = list_get(lista_desplazamientos, 0);
                poner_pagina_en_marco(archivo, pagina, marco, *desplazamiento);
                list_remove(lista_desplazamientos, 0);
                (*paginas_restantes)--;
            }
        } else {
            t_tabla_nivel* subtabla = (t_tabla_nivel*) list_get(tabla->entradas, i);
            escribir_paginas_en_marcos(archivo, subtabla, paginas_restantes, lista_desplazamientos);
        }
    }
}

void poner_pagina_en_marco(FILE* archivo, t_pagina* pagina, int marco, int desplazamiento){
    if (archivo == NULL || marco < 0) {
        log_error(memoria_logger, "Parámetros inválidos");
        return;
    }

    void* buffer_para_lectura = malloc(TAM_PAGINA);

    fseek(archivo, desplazamiento, SEEK_SET);
    fread(buffer_para_lectura, 1, TAM_PAGINA, archivo);

    pthread_mutex_lock(&mutex_espacio_usuario);
    memcpy(espacio_usuario + marco * TAM_PAGINA, buffer_para_lectura, TAM_PAGINA);    
    pthread_mutex_unlock(&mutex_espacio_usuario);

    free(buffer_para_lectura);
    pagina->marco_asignado = marco;
}