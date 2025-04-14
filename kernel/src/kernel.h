#include <utils/commons.h>

t_config* iniciar_config(void);
void iniciar_conexion();
void inicializar_kernel(char* nombre_proceso, char* tamanio_proceso);
t_log* kernel_logger;