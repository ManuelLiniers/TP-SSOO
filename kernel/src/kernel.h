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

    // IO 
void iniciar_servidor_io(void* arg);
void atender_io(void* arg);
void identificar_io(t_buffer* unBuffer,int socket_fd);
    // Dispatch
void iniciar_cpu_dispatch(void* arg);
void atender_dispatch(void* arg);
void modificar_dispatch(cpu* una_cpu,int socket_fd);
    // Interrupt
void iniciar_cpu_interrupt(void* arg);
void atender_interrupt(void* arg);
void modificar_interrupt(cpu* una_cpu, int socket_fd);

void identificar_cpu(t_buffer* buffer, int socket_fd, void (*funcion)(cpu*, int));
bool comparar_cpu_id(cpu* cpu_a, cpu* cpu_b);
cpu* cpu_ya_existe(t_list* cpus, cpu* cpu);

// SEMAFOROS


#endif 