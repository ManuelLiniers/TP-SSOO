#include "../include/marcos.h"


void liberar_marco(uint32_t un_marco){
    pthread_mutex_lock(&mutex_bit_marcos);
    bitarray_clean_bit(bit_marcos, un_marco);
    pthread_mutex_unlock(&mutex_bit_marcos);
}

int obtener_marco_libre(){
    int cantidad_marcos = bitarray_get_max_bit(bit_marcos);
    int marco = -1;

    pthread_mutex_lock(&mutex_bit_marcos);
    for (int i = 0; i < cantidad_marcos; i++) {
        if (!bitarray_test_bit(bit_marcos, i)) {
            bitarray_set_bit(bit_marcos, i);
            marco = i;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_bit_marcos);
    
    return marco;
}

uint32_t cantidad_de_marcos_libres(){
    int cantidad_marcos = bitarray_get_max_bit(bit_marcos);
    uint32_t contador = 0;
    
    pthread_mutex_lock(&mutex_bit_marcos);
    for(int i = 0; i < cantidad_marcos; i++){
        if(!bitarray_test_bit(bit_marcos, i)){
            contador++;
        }
    }
    pthread_mutex_unlock(&mutex_bit_marcos);
    
    return contador;
}

bool hay_marcos_libres() {
    int cantidad_marcos = bitarray_get_max_bit(bit_marcos);
    bool resultado = false;
    
    pthread_mutex_lock(&mutex_bit_marcos);
    for (int i = 0; i < cantidad_marcos; i++) {
        if(!bitarray_test_bit(bit_marcos, i)){
            resultado = true;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_bit_marcos);
    
    return resultado;
}

void destruir_todos_los_marcos() {
    pthread_mutex_lock(&mutex_bit_marcos);
    bitarray_destroy(bit_marcos);
    pthread_mutex_unlock(&mutex_bit_marcos);
}

void asignar_marcos_a_tabla(t_tabla_nivel* tabla, int* paginas_restantes) {
    for (int i = 0; i < ENTRADAS_POR_TABLA && *paginas_restantes > 0; i++) {

        if (tabla->nivel == CANTIDAD_NIVELES) {
            t_pagina* pagina = (t_pagina*) list_get(tabla->entradas, i);

            if (pagina->marco_asignado == -1) {
                int marco = obtener_marco_libre();
                if (marco != -1) {
                    pagina->marco_asignado = marco;
                } else {
                    // No hay más marcos libres (No deberia pasar)
                    log_error(memoria_logger, "No hay marcos libres disponibles para la página %d\n", pagina->nro_pagina);
                }
            }
        } else {
            t_tabla_nivel* subtabla = (t_tabla_nivel*) list_get(tabla->entradas, i);
            asignar_marcos_a_tabla(subtabla, paginas_restantes);
        }
    }
}