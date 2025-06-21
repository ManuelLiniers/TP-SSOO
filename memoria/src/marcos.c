#include "../include/marcos.h"


void liberar_marco(uint32_t un_marco){
    pthread_mutex_lock(&mutex_bit_marcos);
    bitarray_clean_bit(bit_marcos, un_marco);
    pthread_mutex_unlock(&mutex_bit_marcos);
}

int obtener_marco_libre(){
    size_t cantidad_marcos = bitarray_get_max_bit(bit_marcos);
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
    size_t cantidad_marcos = bitarray_get_max_bit(bit_marcos);
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
    size_t cantidad_marcos = bitarray_get_max_bit(bit_marcos);
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

void asignar_marcos_a_tabla(t_tabla_nivel* tabla, int pid) {
    for (int i = 0; i < ENTRADAS_POR_TABLA; i++) {
        t_entrada_tabla* entrada = list_get(tabla->entradas, i);

        if (entrada->es_ultimo_nivel) {
            t_pagina* pagina = (t_pagina*) entrada->siguiente_nivel;
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
            t_tabla_nivel* subtabla = (t_tabla_nivel*) entrada->siguiente_nivel;
            asignar_marcos_a_tabla(subtabla, pid);  // llamada recursiva
        }
    }
}