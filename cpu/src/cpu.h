#include <utils/hello.h>
#include <utils/commons.h>
#include </home/utnso/tp-2025-1c-queCompileALaPrimera/utils/src/utils/socket.h>
#include <instruccion.h>

extern int conexion_memoria;
extern int conexion_kernel_dispatch;
extern int conexion_kernel_interrupt;

t_log* crear_log();
t_log* logger;
t_config* crear_config(t_log* logger);
void mensaje_inicial(int conexion_memoria, int conexion_kernel_dispatch, int conexion_kernel_interrupt);
void terminar_programa(int conexion_memoria, int conexion_kernel_dispatch, int conexion_kernel_interrupt, t_log* logger, t_config* cpu_config);

extern bool flag_interrupcion;
pthread_mutex_t mutex_interrupt;

// Estructura de contexto de ejecución (PCB)
typedef struct {
    int pid;
    int program_counter;
    uint32_t AX;
    uint32_t BX;
    uint32_t CX;
    uint32_t DX;
} t_contexto;

//para identificar casusas por desalojo
typedef enum {
    EXIT,
    IO,
    WAIT,
    SIGNAL,
    PAGE_FAULT,
    INTERRUPCION,
    DESALOJO_POR_QUANTUM
} motivo_desalojo;


// Funciones que vamos a implementar en etapas
void atender_proceso_del_kernel(int fd, t_log* logger);
void destruir_estructuras_del_contexto_actual(t_contexto* contexto);

