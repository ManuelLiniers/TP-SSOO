#ifndef LECTURA_ESCRITURA_H_
#define LECTURA_ESCRITURA_H_

#include "memoria.h"

void* obtener_lectura(uint32_t direccion_fisica, uint32_t tamanio, int pid);
int escribir_espacio(uint32_t direccion_fisica, int tamanio, void* valor, int pid);
int dump_de_memoria(uint32_t pid);
void leer_pagina_completa(uint32_t nro_marco, int socket_destino);
void escribir_pagina_completa(uint32_t nro_marco, void* contenido);

#endif /* LECTURA_ESCRITURA_H_ */