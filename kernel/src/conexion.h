#ifndef CONEXION_H_
#define CONEXION_H_

#include "utils/commons.h"

char* ip_kernel;
char* ip_memoria;
char* puerto_kernel;   //sin hilos hay que cambiarlo manualmente
char* puerto_memoria;
char* puerto_dispatch;
char* puerto_interrupt;
char* puerto_io;
extern int conexion_memoria;
extern t_config *config_kernel;
extern t_log* logger_kernel;

int crear_conexion_memoria();

#endif