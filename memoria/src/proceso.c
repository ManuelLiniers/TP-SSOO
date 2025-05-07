#include </home/utnso/tp-2025-1c-queCompileALaPrimera/memoria/include/proceso.h>

t_proceso* obtener_proceso_por_id(int pid){
	bool _buscar_el_pid(t_proceso* proceso){
		return proceso->pid == pid;
	}
	t_proceso* un_proceso = list_find(list_procss_recibidos, (void*)_buscar_el_pid);
	if(un_proceso == NULL){
		log_error(memoria_logger, "PID<%d> No encontrado en la lista de procesos", pid);
		exit(EXIT_FAILURE);
	}
	return un_proceso;
}