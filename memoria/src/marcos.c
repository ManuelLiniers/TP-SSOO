#include "../include/marcos.h"

t_marco* crear_marco(int base, bool libre, int index){
    t_marco* nuevo_marco = malloc(sizeof(t_marco));
	nuevo_marco->nro_marco = index;
	nuevo_marco->base = base;
	nuevo_marco->libre = libre;
//	nuevo_marco->info_new = NULL;
//	nuevo_marco->info_old = NULL;

//	nuevo_marco->orden_carga = 0;
//	nuevo_marco->ultimo_uso = NULL;

	return nuevo_marco;
}

void liberar_marco(t_marco* un_marco){
    un_marco->libre = true;
	/*un_marco->orden_carga = 0;
	if(un_marco->info_new != NULL){
		free(un_marco->info_new);
		un_marco->info_new = NULL;
	}
	if(un_marco->info_old != NULL){
		free(un_marco->info_old);
		un_marco->info_old = NULL;
	}
	if(un_marco->ultimo_uso != NULL){
		temporal_destroy(un_marco->ultimo_uso);
		un_marco->ultimo_uso = NULL;
	}*/
}

t_marco* obtener_marco_por_nro_marco(int nro_marco){
    t_marco* un_marco;
//	pthread_mutex_lock(&mutex_lst_marco);
	un_marco = list_get(lst_marcos, nro_marco);
//	pthread_mutex_unlock(&mutex_lst_marco);

	return un_marco;
}

void destruir_list_marcos_y_todos_los_marcos(){
    void _destruir_un_marco(t_marco* un_marco){
		/*if(un_marco->info_new != NULL){
			free(un_marco->info_new);
		}
		if(un_marco->info_old != NULL){
			free(un_marco->info_old);
		}
		if(un_marco->ultimo_uso != NULL){
			temporal_destroy(un_marco->ultimo_uso);
		}*/
		free(un_marco);
	}
	list_destroy_and_destroy_elements(lst_marcos, (void*)_destruir_un_marco);
}

