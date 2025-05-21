#ifndef ATENCION_A_CPU_H_
#define ATENCION_A_CPU_H_

#include </home/utnso/tp-2025-1c-queCompileALaPrimera/utils/src/utils/commons.h>
#include <memoria.h>

//============RECIBIDOS DE CPU=======================
void atender_peticion_de_instruccion(t_buffer* un_buffer, int cpu_fd);



//============ENVIOS A CPU=======================

void enviar_instruccion_a_cpu(char* instruccion, int cpu_fd);



#endif /* ATENCION_A_CPU_H_ */