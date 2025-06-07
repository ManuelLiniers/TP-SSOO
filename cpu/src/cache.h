#ifndef CACHE_H
#define CACHE_H
#include <stdint.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <commons/string.h>
#include <stdlib.h>
#include <string.h>
#include "utils/socket.h"



extern t_list* cache_paginas;
extern int entradas_cache;
extern char* algoritmo_cache;
extern int puntero_clock; 

typedef struct {
    int pid;
    uint32_t pagina;
    char* contenido;       // contenido de la página (simulado)
    bool bit_uso;          // para CLOCK y CLOCK-M
    bool bit_modificado;   // solo CLOCK-M
    uint32_t marco;
} t_entrada_cache;

void inicializar_cache(t_log* logger);
bool buscar_en_cache(int pid, uint32_t pagina, char** contenido_resultado, t_log* logger);
void reemplazar_con_clock(t_entrada_cache* nueva, t_log* logger, int conexion_memoria);
void agregar_a_cache(int pid, uint32_t pagina, char* contenido, bool fue_modificado, t_log* logger, int conexion_memoria);
void limpiar_cache_por_pid(int pid, int conexion_memoria, int tamanio_pagina, t_log* logger);




#endif