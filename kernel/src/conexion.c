#include "conexion.h"

char* ip_kernel;
char* ip_memoria;
char* puerto_kernel;   //sin hilos hay que cambiarlo manualmente
char* puerto_memoria;
char* puerto_dispatch;
char* puerto_interrupt;
char* puerto_io;
int conexion_memoria;
t_config *config_kernel;
t_log* logger_kernel;
bool tieneEstimacion;


int crear_conexion_memoria(){
    int conexion = crear_conexion(logger_kernel, ip_memoria, puerto_memoria);
    hacer_handshake(logger_kernel, conexion);

    t_paquete* paqueteID = crear_paquete(IDENTIFICACION);

	void* coso_a_enviar = malloc(sizeof(int));
	int codigo = KERNEL;
	memcpy(coso_a_enviar, &codigo, sizeof(int));
    agregar_a_paquete(paqueteID, coso_a_enviar, sizeof(op_code));
    enviar_paquete(paqueteID, conexion);
    eliminar_paquete(paqueteID);

    return conexion;
}