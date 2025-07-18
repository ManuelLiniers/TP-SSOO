#ifndef VARIABLES_MEMORIA_H_
#define VARIABLES_MEMORIA_H_

#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/bitarray.h>

typedef struct {
    int accesos_tablas_paginas;
    int instrucciones_solicitadas;
    int bajadas_swap;
    int subidas_memoria;
    int lecturas_memoria;
    int escrituras_memoria;
} t_metricas_proceso;

// Estructura de una tabla de nivel
typedef struct {
    int nivel;                // 1 = raíz, 2 = segundo nivel, etc.
    t_list* entradas;             // Lista de t_tabla_nivel* o t_pagina*
} t_tabla_nivel;

// Página virtual de un proceso
typedef struct {
    int nro_pagina;
    int marco_asignado; // En caso de que este en swap el valor es -1
} t_pagina;

typedef struct{
	int pid;
	int paginas; // Cantidad de paginas
	t_metricas_proceso* metricas;

	t_list* instrucciones;

	t_tabla_nivel* tabla_paginas_raiz;  // Puntero a la tabla de nivel 1
} t_proceso;


// Variables de configuración
extern char* PUERTO_ESCUCHA;
extern int TAM_MEMORIA;
extern int TAM_PAGINA;
extern int ENTRADAS_POR_TABLA;
extern int CANTIDAD_NIVELES;
extern int RETARDO_MEMORIA;
extern char* PATH_SWAPFILE;
extern int RETARDO_SWAP;
extern char* LOG_LEVEL;
extern char* DUMP_PATH;
extern char* PATH_INSTRUCCIONES;

// Conexiones
extern int server_fd_memoria;
extern int kernel_fd;
extern int cpu_fd;

// Recursos de ejecución
extern void* espacio_usuario;
extern t_list* procesos_memoria;
extern t_list* procesos_swap;
extern t_list* lista_swap;

// Logs y config
extern t_log* memoria_logger;
extern t_config* memoria_config;
extern t_config* pruebas_config;

// Marcos
extern t_bitarray* bit_marcos;
extern int marcos_totales;

// Mutex
extern pthread_mutex_t m_tablas;
extern pthread_mutex_t mutex_bit_marcos;
extern pthread_mutex_t mutex_espacio_usuario;
extern pthread_mutex_t mutex_procesos;

#endif
