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
void* planificar_corto_plazo_SJF(void* arg);
void* planificar_corto_plazo_SJF_desalojo(void* arg);
void* planificar_largo_plazo_PMCP(void* arg);
void *comprobar_espacio_memoria(void* arg);
bool proceso_es_mas_chico(void* a, void* b);
bool shortest_job_first(void* a, void* b);
void esperar_devolucion_proceso(void* arg);
void esperar_dispatch(void* arg);
void esperar_dispatch_SJF_desalojo(void* arg);
void actualizar_estimaciones();
void log_metricas_estado(t_pcb* proceso);
long calcular_tiempo(t_metricas_estado_tiempo* metrica);
void* planificar_corto_plazo_FIFO(void* arg);
void paquete_memoria_pid(t_pcb* proceso, op_code codigo);

void enviar_proceso_a_io(t_pcb* proceso, int io_id, int io_tiempo);
void vuelta_proceso_io(void* args);
void comprobar_cola_bloqueados(int io_id);
void comprobar_suspendido(t_pcb* proceso);
void notificar_suspension_memoria(t_pcb* proceso);
void verificar_procesos_ready();

bool hay_cpu_libre(t_cpu** cpu_encargada);
bool espacio_en_memoria(t_pcb* proceso);
void poner_en_ready(t_pcb* proceso);
void poner_en_ejecucion(t_pcb* proceso, t_cpu* cpu_encargada, int socket);




#endif // SCHEDULER_H_