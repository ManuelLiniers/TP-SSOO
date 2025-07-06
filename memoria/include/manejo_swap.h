#ifndef MANEJO_SWAP_H_
#define MANEJO_SWAP_H_

#include "memoria.h"

int enviar_a_swap(int pid);
t_list* obtener_espacios_swap(int pid, int cantidad_paginas, bool* paginas_extra);
int primer_hueco_libre();
void escribir_marcos_en_archivo_con_desplazamiento(FILE* archivo, t_tabla_nivel* tabla, int* paginas_restantes, t_list* lista_desplazamientos);
void poner_marco_en_archivo_con_desplazamiento(FILE* archivo, int marco, int desplazamiento);
int sacar_de_swap(int pid);
t_list* obtener_desplazamientos_swap(int pid, int cantidad_paginas);
int primer_pag_archivo(int pid);
void escribir_paginas_en_marcos(FILE* archivo, t_tabla_nivel* tabla, int* paginas_restantes, t_list* lista_desplazamientos);
void poner_pagina_en_marco(FILE* archivo, t_pagina* pagina, int marco, int desplazamiento);

#endif /* MANEJO_SWAP_H_ */