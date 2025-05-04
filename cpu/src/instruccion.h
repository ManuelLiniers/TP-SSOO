#ifndef INSTRUCCION_H
#define INSTRUCCION_H
#include <stdbool.h>

typedef struct {
    char* opcode;
    char** operandos;
    int cantidad_operandos;
    bool necesita_traduccion;
} t_instruccion_decodificada;

t_instruccion_decodificada* decodificar_instruccion(char* instruccion_cruda);
void destruir_instruccion_decodificada(t_instruccion_decodificada* instruccion);

#endif
