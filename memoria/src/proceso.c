#include "../include/proceso.h"

//Falta una funcion y un log que indique la destruccion de un proceso

t_proceso* crear_proceso(int pid, int size, char* path_instruc){
	t_proceso* proceso_nuevo = malloc(sizeof(t_proceso));
	proceso_nuevo->pid = pid;
	proceso_nuevo->size = size;
	proceso_nuevo->pathInstrucciones = path_instruc;
	proceso_nuevo->instrucciones = NULL;

//  Cargo las instrucciones del proceso
	proceso_nuevo->instrucciones = leer_archivo_y_cargar_instrucciones(proceso_nuevo->pathInstrucciones);

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

        log_info(memoria_logger, "Instrucción: [%s]", linea_instruccion);

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
	char* instruccion_actual;
	if(indice_instruccion >= 0 && indice_instruccion < list_size(un_proceso->instrucciones)){
		instruccion_actual = list_get(un_proceso->instrucciones, indice_instruccion);
		return instruccion_actual;
	}
	else{
		log_error(memoria_logger, "PID<%d> - Nro de Instruccion <%d> NO VALIDA", un_proceso->pid, indice_instruccion);
		exit(EXIT_FAILURE);
		return NULL;
	}
}