#include "../include/proceso.h"

//Falta una funcion y un log que indique la destruccion de un proceso

t_proceso* crear_proceso(int pid, int size, char* path_instruc){
	t_proceso* proceso_nuevo = malloc(sizeof(t_proceso));
	proceso_nuevo->pid = pid;
	proceso_nuevo->size = size;
    t_metricas_proceso* metricas = calloc(1, sizeof(t_metricas_proceso));
    proceso_nuevo->metricas = metricas; 

//  Cargo las instrucciones del proceso
	proceso_nuevo->instrucciones = leer_archivo_y_cargar_instrucciones(path_instruc);

    int paginas = size/TAM_PAGINA + 1;
    int contador = 0;
    proceso_nuevo->tabla_paginas_raiz = crear_tabla_multinivel(1, &paginas, &contador);
    pthread_mutex_init(&proceso_nuevo->mutex_TP, NULL);

	return proceso_nuevo;
}

void agregar_proceso_a_lista(t_proceso* un_proceso ,t_list* una_lista) {
	list_add(una_lista, un_proceso);
}

t_list* leer_archivo_y_cargar_instrucciones(char* path_archivo) {
    FILE* archivo = fopen(path_archivo, "rt");
    if (archivo == NULL) {
        log_error(memoria_logger, "No se encontró el archivode instrucciones");
        return NULL;
    }

    t_list* instrucciones = list_create();
    char linea_instruccion[256];

    while (fgets(linea_instruccion, sizeof(linea_instruccion), archivo)) {
        // Remover salto de línea si lo hay
        linea_instruccion[strcspn(linea_instruccion, "\n")] = '\0';

        log_debug(memoria_logger, "Instrucción: [%s]", linea_instruccion);

        char** tokens = string_split(linea_instruccion, " ");

        int i = 0;
        while (tokens[i]) i++;

        // Armar instrucción formateada como string
        char* instruccion_formateada = NULL;
        if (i == 3) {
            instruccion_formateada = string_from_format("%s %s %s", tokens[0], tokens[1], tokens[2]);
        } else if (i == 2) {
            instruccion_formateada = string_from_format("%s %s", tokens[0], tokens[1]);
        } else {
            instruccion_formateada = strdup(tokens[0]);
        }

        list_add(instrucciones, instruccion_formateada);

        // Liberar tokens
        for (int j = 0; j < i; j++) {
            free(tokens[j]);
        }
        free(tokens);
    }

    fclose(archivo);
    return instrucciones;
}

t_proceso* obtener_proceso_por_id(int pid){
    bool _buscar_el_pid(t_proceso* proceso){
		return proceso->pid == pid;
	};
	t_proceso* un_proceso = list_find(procesos_memoria, (void*)_buscar_el_pid);
	if(un_proceso == NULL){
		log_error(memoria_logger, "PID<%d> No encontrado en la lista de procesos", pid);
		exit(EXIT_FAILURE);
	}
	return un_proceso;
}


char* obtener_instruccion_por_indice(t_proceso* un_proceso, int indice_instruccion){
	if (un_proceso == NULL || un_proceso->instrucciones == NULL) {
        log_error(memoria_logger, "Proceso o lista de instrucciones es NULL");
        exit(EXIT_FAILURE);
    }

	if(indice_instruccion >= 0 && indice_instruccion < list_size(un_proceso->instrucciones)){
        un_proceso->metricas->instrucciones_solicitadas++;
		return list_get(un_proceso->instrucciones, indice_instruccion);
	}
	else{
		log_error(memoria_logger, "PID<%d> - Nro de Instruccion <%d> NO VALIDA", un_proceso->pid, indice_instruccion);
		exit(EXIT_FAILURE);
	}
}

void exponer_metricas(t_metricas_proceso* metricas, uint32_t pid){
    log_info(memoria_logger, "## PID: %d - Proceso Destruido - Métricas - Acc.T.Pag: %d; Inst.Sol.: %d; SWAP: %d; Mem.Prin.: %d; Lec.Mem.: %d; Esc.Mem.: %d", 
          pid, metricas->accesos_tablas_paginas, metricas->instrucciones_solicitadas, 
          metricas->bajadas_swap, metricas->subidas_memoria,
          metricas->lecturas_memoria, metricas->escrituras_memoria);
}

void finalizar_proceso(t_proceso* proceso){
    pthread_mutex_lock(&proceso->mutex_TP);

    liberar_tablas(proceso->tabla_paginas_raiz);

    list_destroy_and_destroy_elements(proceso->instrucciones, free);
    free(proceso->metricas);

    pthread_mutex_unlock(&proceso->mutex_TP);
    pthread_mutex_destroy(&proceso->mutex_TP);
    free(proceso);
}