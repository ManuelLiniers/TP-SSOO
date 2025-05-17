#include <utils/commons.h>
#include <pcb.h>
#include <scheduler.h>
#include<semaforos.h>

char* ip_kernel;
char* ip_memoria;
char* puerto_kernel;   //sin hilos hay que cambiarlo manualmente
char* puerto_memoria;
char* puerto_dispatch;
char* puerto_interrupt;
char* puerto_io;
t_config *config_kernel;
t_log* logger_kernel;

t_config* iniciar_config(void);
void iniciar_conexion();
void inicializar_kernel();
void inicializar_planificacion();

// SEMAFOROS


