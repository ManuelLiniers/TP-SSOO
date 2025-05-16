#include <commons/config.h>
#include <utils/commons.h>

typedef struct {
    int pid;
    int tiempo;
} t_peticion_io;

typedef enum {
    MENSAJE,
    PETICION_IO
    // Podés agregar más op_codes si los necesitás
} op_code;

t_config* iniciar_config(void);
void procesar_io(int socket_kernel, t_log* logger);
t_peticion_io* deserializar_peticion_io(void* stream);
void terminar_programa(t_log*, t_config*, int);