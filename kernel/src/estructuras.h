#ifndef PCB_H_
#define PCB_H_

#include <commons/collections/list.h>
#include "utils/commons.h"
#include "semaforos.h"
#include "conexion.h"

void mostrar_cola(t_queue** cola);
void mostrar_lista(t_list* lista);
void mostrar_cpus();

extern int pid_incremental;
extern int id_io_incremental;

extern t_list* lista_dispositivos_io;
void iniciar_dispositivos_io();
extern t_list* lista_cpus;
void iniciar_cpus();
extern int estimacion_inicial;
extern double estimador_alfa;

// Estados posibles de un proceso
typedef enum { NEW, READY, EXEC, BLOCKED, SUSP_READY, SUSP_BLOCKED, EXIT } t_estado;

int id_estado(t_estado estado);

extern t_queue* queue_new;
extern t_list* queue_new_PMCP;
extern t_queue* queue_ready;
extern t_list* queue_ready_SJF;
extern t_list* queue_block;
extern t_queue* queue_exit;

typedef struct{
    t_estado estado;
    double tiempo_inicio;
    double tiempo_fin;
 } t_metricas_estado_tiempo;

typedef struct{
    int cpu_id;
    int socket_dispatch;
    int socket_interrupt;
    bool esta_libre;
} t_cpu;

t_cpu* buscar_cpu_libre(t_list* lista_cpus);
void mostrar_cpu(t_cpu* cpu);


// Control Block de un proceso
typedef struct {
    int pid;                    // Identificador único
    int program_counter;        // Contador de instrucción
    char* instrucciones;      // Lista de instrucciones (strings o structs)
    t_estado estado;            // Estado actual del proceso
    int tamanio_proceso; 
    int metricas_estado[5];         // Lista de veces que estuvo en cada estado
    t_list* metricas_tiempo;  // Lista de tiempo que estuvo en cada estado
    int registros[4];
    long estimacion_anterior;
    long rafaga_real;
    long estimacion_actual;
    // Más campos opcionales: tamaño de memoria, registros, métricas, etc.
} t_pcb;

void crear_proceso(char* instrucciones, char* tamanio_proceso);

t_pcb* buscar_proceso_pid(uint32_t pid);
void cambiarEstado(t_pcb* proceso, t_estado estado);
void calcular_estimacion(t_pcb* proceso);
char* estado_to_string(t_estado estado);

typedef struct{
    void (*funcion_agregacion)(void* cola_ready, t_pcb* proceso);
    void* cola_ready;
} t_agregacion_ready;

void agregar_a_lista(void* cola_ready, t_pcb* proceso);
void agregar_a_cola(void* cola_ready, t_pcb* proceso);

typedef struct {
    t_pcb* proceso;
    t_cpu* cpu;
} t_unidad_ejecucion;

extern t_list* lista_procesos_ejecutando;
void sacar_proceso_ejecucion(t_pcb* proceso);

t_metricas_estado_tiempo* obtener_ultima_metrica(t_pcb* proceso);
t_list* obtener_exec(t_pcb* proceso);
long calcular_rafaga(t_list* estados_exec);


/**
 * @brief Crea un PCB inicializado
 * @param pid Identificador del proceso
 * @param instrucciones Lista de instrucciones (creada externamente)
 * @return puntero a t_pcb o NULL en caso de error
 */
t_pcb* pcb_create();

/**
 * @brief Destruye un PCB liberando instrucciones y la propia estructura
 * @param pcb_void puntero a t_pcb (pasado como void* para destruir en queues)
 */
void pcb_destroy(void* pcb_void);

typedef struct{
    char nombre[20];
    int id;
    int socket;
} t_dispositivo_io;

t_dispositivo_io* buscar_io(int id_io);
t_queue* obtener_cola_io(int io_id);

typedef struct{
    t_pcb *pcb;
    int tiempo;
} tiempo_en_io;

#endif // PCB_H_
