#ifndef VARIABLES_MEMORIA_H_
#define VARIABLES_MEMORIA_H_

#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>

typedef struct {
    int accesos_tablas_paginas;
    int instrucciones_solicitadas;
    int bajadas_swap;
    int subidas_memoria;
    int lecturas_memoria;
    int escrituras_memoria;
} t_metricas_proceso;

// Entrada de tabla de páginas
typedef struct {
    int numero_entrada;
    void* siguiente_nivel; // t_tabla_nivel* o t_pagina*
    bool es_ultimo_nivel;
} t_entrada_tabla;

// Estructura de una tabla de nivel
typedef struct {
    int nivel;                // 1 = raíz, 2 = segundo nivel, etc.
    t_list* entradas;             // Lista de t_entrada_tabla*
} t_tabla_nivel;

// Página virtual de un proceso
typedef struct {
    int nro_pagina;
    int marco_asignado; // En caso de que este en swap el valor es -1
} t_pagina;

typedef struct{
	int pid;
	int size;
	t_metricas_proceso* metricas;

	t_list* instrucciones;

	t_tabla_nivel* tabla_paginas_raiz;  // Puntero a la tabla de nivel 1
	pthread_mutex_t mutex_TP;
} t_proceso;

typedef struct {
	int pid_proceso;
	int nro_pagina;
} marco_info;

typedef struct {
    int nro_marco;
    int base;
    bool libre;
    marco_info* info;
} t_marco;

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

// Logs y config
extern t_log* memoria_logger;
extern t_config* memoria_config;

// Paginas
extern t_list* lst_marcos;
extern int marcos_totales;

// Semaforos
extern pthread_mutex_t m_tablas;
extern pthread_mutex_t mutex_lst_marco;
extern pthread_mutex_t mutex_espacio_usuario;

#endif
