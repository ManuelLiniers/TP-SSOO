#ifndef ATENCION_A_KERNEL_H_
#define ATENCION_A_KERNEL_H_

#include "utils/commons.h"
#include "memoria.h"

void iniciar_proceso(t_buffer* unBuffer, int kernel_fd);
void fin_proceso(t_buffer* unBuffer, int kernel_fd);
void atender_dump_memory(t_buffer* unBuffer, int cpu_fd);
void atender_swap(t_buffer* unBuffer, int cpu_fd);

#endif /* ATENCION_A_KERNEL_H_ */