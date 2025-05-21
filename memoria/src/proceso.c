#include "../include/proceso.h"


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
