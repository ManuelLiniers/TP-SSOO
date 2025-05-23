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


int crear_conexion_memoria(){
    return crear_conexion(logger_kernel, ip_memoria, puerto_memoria);
}