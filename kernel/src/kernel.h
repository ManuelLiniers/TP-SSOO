#ifndef KERNEL_H_
#define KERNEL_H_

#include "utils/commons.h"
#include "estructuras.h"
#include <scheduler.h>
#include <semaforos.h>

void iniciar_config(char* pruebas);
void iniciar_conexion();
void inicializar_kernel();
void inicializar_planificacion();
void inicializar_servidores();

    // IO 
void* iniciar_servidor_io();
void* atender_io(void* arg);
void* identificar_io(t_buffer* unBuffer,int socket_fd);
    // Dispatch
void* iniciar_cpu_dispatch(void* arg);
void* atender_dispatch(void* arg);
void modificar_dispatch(t_cpu* una_cpu,int socket_fd);
void* identificar_cpu_distpatch(t_buffer* buffer, int socket);
    // Interrupt
void* iniciar_cpu_interrupt(void* arg);
void* atender_interrupt(void* arg);
void modificar_interrupt(t_cpu* una_cpu, int socket_fd);
void* identificar_cpu_interrupt(t_buffer* buffer, int socket);

t_cpu* identificar_cpu(t_buffer* buffer, int socket_fd, void (*funcion)(t_cpu*, int));
bool comparar_cpu_id(t_cpu* cpu_a, t_cpu* cpu_b);
t_cpu* cpu_ya_existe(t_list* cpus, t_cpu* cpu);

void* escuchar_socket_io(void* arg);

void* esperar_dispatch(void* arg);

void crear_proceso(char* instrucciones, int tamanio_proceso);
void finalizar_proceso(t_pcb* proceso);
void finalizar_proceso_io(void* arg);
void finalizar_unidad_ejecucion(void* arg);
void matar_proceso(void* arg);

// SEMAFOROS


#endif 