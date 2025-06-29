#include "../include/variables_memoria.h"

char* PUERTO_ESCUCHA;
int TAM_MEMORIA;
int TAM_PAGINA;
int ENTRADAS_POR_TABLA;
int CANTIDAD_NIVELES;
int RETARDO_MEMORIA;
char* PATH_SWAPFILE;
int RETARDO_SWAP;
char* LOG_LEVEL;
char* DUMP_PATH;
char* PATH_INSTRUCCIONES;

int server_fd_memoria;
int kernel_fd;
int cpu_fd;

void* espacio_usuario;
t_list* procesos_memoria;
t_list* lista_swap;

t_log* memoria_logger;
t_config* memoria_config;

t_bitarray* bit_marcos;
int marcos_totales;

pthread_mutex_t m_tablas;
pthread_mutex_t mutex_bit_marcos;
pthread_mutex_t mutex_espacio_usuario;
