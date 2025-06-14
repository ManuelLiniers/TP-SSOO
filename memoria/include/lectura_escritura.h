#ifndef LECTURA_ESCRITURA_H_
#define LECTURA_ESCRITURA_H_

#include "memoria.h"

void* obtener_lectura(uint32_t direccion_fisica, uint32_t tamanio);
int escribir_espacio(uint32_t direccion_fisica, int tamanio, void* valor);

#endif /* LECTURA_ESCRITURA_H_ */