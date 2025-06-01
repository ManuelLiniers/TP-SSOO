#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <commons/collections/queue.h>
#include "estructuras.h"
#include "semaforos.h"
#include "conexion.h"


/**
 * @brief Inicializa las cuatro colas del scheduler
 */
void scheduler_init(void);

/**
 * @brief Destruye las colas liberando también los PCBs que contienen
 */
void scheduler_destroy(void);

extern char* algoritmo_corto_plazo;
extern char* algoritmo_largo_plazo;

void* planificar_largo_plazo_FIFO(void* arg);
void* planificar_largo_plazo_PMCP(void* arg);
void *comprobar_espacio_memoria(void* arg);
bool proceso_es_mas_chico(void* a, void* b);
void esperar_devolucion_proceso(void* arg);
void esperar_dispatch(void* arg);
void esperar_interrupt(void* arg);
void* planificar_corto_plazo(void* arg);

void enviar_proceso_a_io(t_pcb* proceso, int io_id, int io_tiempo);
void vuelta_proceso_io(void* args);
t_queue* obtener_cola_io(int io_id);
void comprobar_cola_bloqueados(int io_id);

t_pcb* buscar_proceso_pid(uint32_t pid);

void cambiarEstado(t_pcb* proceso,t_estado EXEC);
bool espacio_en_memoria(t_pcb* proceso);
void poner_en_ready(t_pcb* proceso);
void poner_en_ejecucion(t_pcb* proceso, t_cpu** cpu_encargada);
t_dispositivo_io* buscar_io(int id_io);
t_cpu* buscar_cpu_libre(t_list* lista_cpus);



#endif // SCHEDULER_H_