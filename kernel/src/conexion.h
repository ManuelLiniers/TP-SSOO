#ifndef CONEXION_H_
#define CONEXION_H_

#include "utils/commons.h"

extern char* ip_kernel;
extern char* ip_memoria;
extern char* puerto_kernel;   //sin hilos hay que cambiarlo manualmente
extern char* puerto_memoria;
extern char* puerto_dispatch;
extern char* puerto_interrupt;
extern char* puerto_io;
extern int conexion_memoria;
extern t_config *config_kernel;
extern t_log* logger_kernel;
extern char* algoritmo_corto_plazo;
extern char* algoritmo_largo_plazo;

int crear_conexion_memoria();

#endif