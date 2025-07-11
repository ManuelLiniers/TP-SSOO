#include "../include/proceso.h"

//Falta una funcion y un log que indique la destruccion de un proceso

t_proceso* crear_proceso(int pid, int size, char* path_instruc){
	t_proceso* proceso_nuevo = malloc(sizeof(t_proceso));
	proceso_nuevo->pid = pid;
    t_metricas_proceso* metricas = calloc(1, sizeof(t_metricas_proceso));
    proceso_nuevo->metricas = metricas; 

//  Cargo las instrucciones del proceso
	proceso_nuevo->instrucciones = leer_archivo_y_cargar_instrucciones(path_instruc);

    if(size != 0){
        int paginas = size/TAM_PAGINA + 1;
	    proceso_nuevo->paginas = paginas;
        int contador = 0;
        proceso_nuevo->tabla_paginas_raiz = crear_tabla_multinivel(1, &paginas, &contador);
    } else {
        proceso_nuevo->paginas = 0;
        proceso_nuevo->tabla_paginas_raiz = NULL;
    }

	return proceso_nuevo;
}

void agregar_proceso_a_lista(t_proceso* un_proceso ,t_list* una_lista) {
	list_add(una_lista, un_proceso);
}

t_list* leer_archivo_y_cargar_instrucciones(char* nombre_proceso) {
    char path_archivo[256];
    snprintf(path_archivo, sizeof(path_archivo), "%s%s", PATH_INSTRUCCIONES, nombre_proceso);
    log_debug(memoria_logger, "Path:%s", path_archivo);
    FILE* archivo = fopen(path_archivo, "r");
    if (archivo == NULL) {
        log_error(memoria_logger, "No se encontró el archivo de instrucciones");
        return NULL;
    }


    t_list* lista_instrucciones = list_create();
    char linea[200];
    
    while (fgets(linea, sizeof(linea), archivo)) {
        if (linea[strlen(linea)-1] == '\n') {
            linea[strlen(linea)-1] = '\0';
        }
        
        char* instruccion = malloc(strlen(linea) + 1);
        strcpy(instruccion, linea);
        
        list_add(lista_instrucciones, instruccion);
        
        log_debug(memoria_logger, "Leída instrucción: %s", instruccion);
    }

    fclose(archivo);
    return lista_instrucciones;
}

t_proceso* obtener_proceso_por_id(int pid, t_list* lista_procesos){
    bool buscar_el_pid(t_proceso* proceso){
		return proceso->pid == pid;
	};
	t_proceso* un_proceso = list_find(lista_procesos, (void*)buscar_el_pid);
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

    liberar_tablas(proceso->tabla_paginas_raiz);

    list_destroy_and_destroy_elements(proceso->instrucciones, free);
    free(proceso->metricas);

    free(proceso);
}