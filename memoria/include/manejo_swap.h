#ifndef MANEJO_SWAP_H_
#define MANEJO_SWAP_H_

#include "memoria.h"

void enviar_a_swap(int pid);
t_list* obtener_espacios_swap(int pid, int cantidad_paginas);
int primer_hueco_libre();
void escribir_marcos_en_archivo_con_desplazamiento(FILE* archivo, t_tabla_nivel* tabla, int* paginas_restantes, t_list* lista_desplazamientos);
void poner_marco_en_archivo_con_desplazamiento(FILE* archivo, int marco, int desplazamiento);

#endif /* MANEJO_SWAP_H_ */