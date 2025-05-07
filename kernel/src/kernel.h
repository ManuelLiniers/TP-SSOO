#include <utils/commons.h>
#include <pcb.h>
#include <scheduler.h>

char* IP_KERNEL = "127.0.0.1";
char* PUERTO_KERNEL = "8003";   //sin hilos hay que cambiarlo manualmente
t_config *config_kernel;
t_log* logger_kernel;

t_config* iniciar_config(void);
void iniciar_conexion();
void inicializar_kernel();
void inicializar_planificacion(char* nombre_proceso, char* tamanio_proceso);
