#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>

t_config* iniciar_config(void);
void iniciar_conexion();
void inicializar_kernel(char* nombre_proceso, char* tamanio_proceso);