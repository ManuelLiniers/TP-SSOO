#include <utils/commons.h>

char* IP_KERNEL = "127.0.0.1";
char* PUERTO_KERNEL = "8003";   //sin hilos hay que cambiarlo manualmente

t_config* iniciar_config(void);
void iniciar_conexion();
void inicializar_kernel(char* nombre_proceso, char* tamanio_proceso, t_log* kernel_logger);
t_log* kernel_logger;