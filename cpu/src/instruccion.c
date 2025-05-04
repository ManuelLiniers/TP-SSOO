#include "instruccion.h"
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>

t_instruccion_decodificada* decodificar_instruccion(char* instruccion_cruda) {
    char** tokens = string_split(instruccion_cruda, " ");
    if (!tokens) return NULL;

    t_instruccion_decodificada* instruccion = malloc(sizeof(t_instruccion_decodificada));
    instruccion->opcode = strdup(tokens[0]);

    // Contar cantidad de operandos
    int cantidad = 0;
    for (int i = 1; tokens[i] != NULL; i++) cantidad++;

    instruccion->cantidad_operandos = cantidad;
    instruccion->operandos = malloc(sizeof(char*) * cantidad);

    for (int i = 0; i < cantidad; i++) {
        instruccion->operandos[i] = strdup(tokens[i + 1]);
    }

    // Determinar si requiere traducción de dirección
    if (string_equals_ignore_case(instruccion->opcode, "READ") ||
        string_equals_ignore_case(instruccion->opcode, "WRITE")) {
        instruccion->necesita_traduccion = true;
    } else {
        instruccion->necesita_traduccion = false;
    }

    string_array_destroy(tokens);
    return instruccion;
}


void destruir_instruccion_decodificada(t_instruccion_decodificada* instruccion) {
    free(instruccion->opcode);
    for (int i = 0; i < instruccion->cantidad_operandos; i++) {
        free(instruccion->operandos[i]);
    }
    free(instruccion->operandos);
    free(instruccion);
}
