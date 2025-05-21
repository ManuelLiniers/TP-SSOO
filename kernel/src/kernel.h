#ifndef KERNEL_H_
#define KERNEL_H_

#include "utils/commons.h"
#include "estructuras.h"
#include "scheduler.h"
#include "semaforos.h"

void iniciar_config();
void iniciar_conexion();
void inicializar_kernel();
void inicializar_planificacion();
void inicializar_servidores();
void iniciar_servidor_io(void* arg);
void atender_io(void* arg);
void identificar_io(t_buffer* unBuffer,int socket_fd);

// SEMAFOROS


#endif 