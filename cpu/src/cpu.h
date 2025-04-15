#include <utils/hello.h>
#include <utils/commons.h>
#include</home/utnso/tp-2025-1c-queCompileALaPrimera/utils/socket.h>

t_log* crear_log();
t_config* crear_config();
void mensaje_inicial(int conexion_memoria, int conexion_kernel_dispatch, int conexion_kernel_interrupt);
void terminar_programa(int conexion_memoria, int conexion_kernel_dispatch, int conexion_kernel_interrupt, t_log* logger, t_config* cpu_config);