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
} t_entrada_cache;

#endif