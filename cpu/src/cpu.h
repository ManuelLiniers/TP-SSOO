#ifndef CPU_H_
#define CPU_H_

#include "utils/hello.h"
#include "utils/commons.h"
#include "utils/socket.h"
#include "instruccion.h"
#include "tlb.h"
#include "cache.h"
#include <math.h>

extern int conexion_memoria;
extern int conexion_kernel_dispatch;
extern int conexion_kernel_interrupt;
extern int TAMANIO_PAGINA;
extern t_config* cpu_config;
extern t_log* logger;

t_log* crear_log();
t_config* crear_config(t_log* logger);
void mensaje_inicial(int conexion_memoria, int conexion_kernel_dispatch, int conexion_kernel_interrupt);
void terminar_programa(int conexion_memoria, int conexion_kernel_dispatch, int conexion_kernel_interrupt, t_log* logger, t_config* cpu_config);

extern bool flag_interrupcion;
extern pthread_mutex_t mutex_interrupt;

// Estructura de contexto de ejecucion (PCB)
typedef struct {
    int pid;
    int program_counter;
    uint32_t AX;
    uint32_t BX;
    uint32_t CX;
    uint32_t DX;
} t_contexto;


void atender_proceso_del_kernel(t_contexto* contexto, t_log* logger);
void destruir_estructuras_del_contexto_actual(t_contexto* contexto);
void* escuchar_interrupt(void* arg);
char* ciclo_de_instruccion_fetch(int conexion_memoria, t_contexto* contexto);
char* recibir_instruccion(int socket_memoria);
t_instruccion_decodificada* ciclo_de_instruccion_decode(char* instruccion_cruda);
void ciclo_de_instruccion_execute(t_instruccion_decodificada* instruccion, t_contexto* contexto, t_log* logger, int conexion_memoria);
uint32_t traducir_direccion_logica(uint32_t direccion_logica, int tamanio_pagina, t_contexto* contexto, int conexion_memoria);
bool hay_interrupcion();
void enviar_contexto_a_kernel(t_contexto* contexto, motivo_desalojo motivo, int fd, t_log* logger); 
void enviar_contexto_a_kernel_io(t_contexto* contexto, motivo_desalojo motivo, int fd, t_log* logger, int id_io, int tiempo_io);
void enviar_contexto_a_kernel_init_proc(t_contexto* contexto, motivo_desalojo motivo, int fd, t_log* logger, char* archivo, int tamanio);
t_buffer* recibir_buffer_contexto(int socket);
t_contexto* deserializar_contexto(t_buffer* buffer);
void abrir_conexion_memoria(int conexion);
void abrir_conexion_kernel(int conexion);

#endif /* CPU_H_ */