#include <cpu.h>

int main(int argc, char* argv[]) {
    saludar("cpu");

    t_log* logger = crear_log();
    t_config* cpu_config = crear_config(logger);

    char* ip_memoria = config_get_string_value(cpu_config, "IP_MEMORIA");
	char* puerto_memoria = config_get_string_value(cpu_config, "PUERTO_MEMORIA");

    char* ip_kernel = config_get_string_value(cpu_config, "IP_KERNEL");
	char* puerto_kernel_dispatch = config_get_string_value(cpu_config, "PUERTO_KERNEL_DISPATCH");
	char* puerto_kernel_interrupt = config_get_string_value(cpu_config, "PUERTO_KERNEL_INTERRUPT");

    int conexion_memoria = crear_conexion(logger, ip_memoria, puerto_memoria);
    int conexion_kernel_dispatch = crear_conexion(logger, ip_kernel, puerto_kernel_dispatch);
    int conexion_kernel_interrupt = crear_conexion(logger, ip_kernel, puerto_kernel_interrupt);

    mensaje_inicial(conexion_memoria, conexion_kernel_dispatch, conexion_kernel_interrupt);

    //espero PCBs del Kernel por dispatch
while(1) {
    int cliente_fd = esperar_cliente(logger, conexion_kernel_dispatch); 
    if(cliente_fd != -1) {
        atender_proceso_del_kernel(cliente_fd, logger);
    }
}
    terminar_programa(conexion_memoria, conexion_kernel_dispatch, conexion_kernel_interrupt, logger, cpu_config);

    return 0;
}


t_log* crear_log(){

    t_log* logger = log_create("cpu.log", "[CPU]", 1, LOG_LEVEL_DEBUG);
    if(logger == NULL){
        perror("error al crear el logger");
        abort();
    } 
    return logger;
}

t_config* crear_config(t_log* logger){

    t_config* cpu_config = config_create("/home/utnso/tp-2025-1c-queCompileALaPrimera/cpu/cpu.config");
    if(cpu_config == NULL){
        log_error(logger, "error con el config del cpu");
        abort();
    }
    return cpu_config;
}

void mensaje_inicial(int conexion_memoria, int conexion_kernel_dispatch, int conexion_kernel_interrupt){
    enviar_mensaje("Conexion inicial de la CPU", conexion_memoria);
    enviar_mensaje("Conexion inicial de la CPU - Dispatch", conexion_kernel_dispatch);
    enviar_mensaje("Conexion inicial de la CPU - Interrumpt", conexion_kernel_interrupt);
}

void terminar_programa(int conexion_memoria, int conexion_kernel_dispatch, int conexion_kernel_interrupt, t_log* logger, t_config* cpu_config)
{
    log_info(logger, "cliente cpu terminado");
	log_destroy(logger);
    config_destroy(cpu_config);
    close(conexion_memoria);
    close(conexion_kernel_dispatch);
    close(conexion_kernel_interrupt);
}

void atender_proceso_del_kernel(int fd, t_log* logger) {
    log_info(logger, "Esperando contexto del proceso desde el Kernel...");

    t_contexto* contexto = recibir_contexto(fd);
    if (!contexto) {
        log_error(logger, "Fallo al recibir el contexto.");
        return;
    }

    while (1) {
        ciclo_de_instruccion_fetch(logger);
        ciclo_de_instruccion_decode(logger);
        ciclo_de_instruccion_execute(logger);

        if (contexto->program_counter == -1) {
            break;
        }
    }

    destruir_estructuras_del_contexto_actual(contexto);
}

t_contexto* recibir_contexto(int fd) {
    t_contexto* contexto = malloc(sizeof(t_contexto));
    contexto->program_counter = 0;
    contexto->AX = 0;
    contexto->BX = 0;
    contexto->CX = 0;
    contexto->DX = 0;
    return contexto;
}

void ciclo_de_instruccion_fetch(t_log* logger) {
    log_debug(logger, "Fetch de instrucción.");
}

void ciclo_de_instruccion_decode(t_log* logger) {
    log_debug(logger, "Decode de instrucción.");
}

void ciclo_de_instruccion_execute(t_log* logger) {
    log_debug(logger, "Execute de instrucción.");
}

void destruir_estructuras_del_contexto_actual(t_contexto* contexto) {
    free(contexto);
}