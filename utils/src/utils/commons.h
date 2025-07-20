#ifndef COMMONS_H_
#define COMMONS_H_

#include <commons/log.h>
#include <readline/readline.h>
#include <commons/config.h>
#include <commons/collections/queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <commons/temporal.h>
#include "socket.h"

typedef enum {
    KERNEL,
    CPU,
    IO,
    MEMORIA
} modulo_id;

//para identificar casusas por desalojo
typedef enum {
    FINALIZADO,
    CAUSA_IO,
    WAIT,
    SIGNAL,
    PAGE_FAULT,
    INTERRUPCION,
    DESALOJO_POR_QUANTUM,
    CAUSA_INIT_PROC,
    EXEC_INSTRUC,
    MEMORY_DUMP
} motivo_desalojo;

#endif
