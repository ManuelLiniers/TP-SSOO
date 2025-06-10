#ifndef ATENCION_A_CPU_H_
#define ATENCION_A_CPU_H_

#include "utils/commons.h"
#include "memoria.h"

void atender_peticion_de_instruccion(t_buffer* un_buffer, int cpu_fd);
void enviar_instruccion_a_cpu(char* instruccion, int cpu_fd);

void atender_peticion_marco(t_buffer* unBuffer, int cpu_fd);



#endif /* ATENCION_A_CPU_H_ */