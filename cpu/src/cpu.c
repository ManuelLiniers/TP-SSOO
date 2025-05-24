#include "cpu.h"

int conexion_memoria;
int conexion_kernel_dispatch;
int conexion_kernel_interrupt;

int main(int argc, char* argv[]) {
    saludar("cpu");

    t_log* logger = crear_log();
    t_config* cpu_config = crear_config(logger);


    int TAMANIO_PAGINA = config_get_int_value(cpu_config, "TAMANIO_PAGINA");
    bool flag_interrupcion = false;
    pthread_mutex_init(&mutex_interrupt, NULL);


    char* ip_memoria = config_get_string_value(cpu_config, "IP_MEMORIA");
	char* puerto_memoria = config_get_string_value(cpu_config, "PUERTO_MEMORIA");

    char* ip_kernel = config_get_string_value(cpu_config, "IP_KERNEL");
	char* puerto_kernel_dispatch = config_get_string_value(cpu_config, "PUERTO_KERNEL_DISPATCH");
	char* puerto_kernel_interrupt = config_get_string_value(cpu_config, "PUERTO_KERNEL_INTERRUPT");

    conexion_memoria = crear_conexion(logger, ip_memoria, puerto_memoria);
    conexion_kernel_dispatch = crear_conexion(logger, ip_kernel, puerto_kernel_dispatch);
    conexion_kernel_interrupt = crear_conexion(logger, ip_kernel, puerto_kernel_interrupt);

    mensaje_inicial(conexion_memoria, conexion_kernel_dispatch, conexion_kernel_interrupt);


pthread_t hilo_interrupt;
pthread_create(&hilo_interrupt, NULL, escuchar_interrupt, &conexion_kernel_interrupt);
pthread_detach(hilo_interrupt);

    //espero PCBs del Kernel por dispatch
while(1) {
    int cliente_fd = esperar_cliente(logger, "KERNEL", conexion_kernel_dispatch); 
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

void atender_proceso_del_kernel(int fd, t_log* logger) {   //EJECUTA CICLO DE INSTRUCCION ENVIADA DESDE MEMORIA, fd es la conexion a memoria
    log_info(logger, "Esperando contexto del proceso desde el Kernel...");

    t_contexto* contexto = recibir_contexto(conexion_kernel_dispatch);  //recibo pcb (t_contexto) de kernel
    if (!contexto) {
        log_error(logger, "Fallo al recibir el contexto.");
        return;
    }

    while (1) {
    char* instruccion_cruda = ciclo_de_instruccion_fetch(fd, contexto); 
    if (!instruccion_cruda) {
        log_error(logger, "Fallo al recibir la instrucción desde Memoria.");
        break;
    }

    t_instruccion_decodificada* instruccion = ciclo_de_instruccion_decode(instruccion_cruda);
    free(instruccion_cruda); // Se libera la instrucción "cruda" después del decode

    if (!instruccion) {
        log_error(logger, "Fallo al decodificar la instrucción.");
        break;
    }

    ciclo_de_instruccion_execute(instruccion, contexto, logger, fd); 
    if (hay_interrupcion()) {
        log_info(logger, "se detectó una interrupción luego de ejecutar la instrucción");
        enviar_contexto_a_kernel(contexto, INTERRUPCION, conexion_kernel_dispatch, logger);  

        pthread_mutex_lock(&mutex_interrupt);
        flag_interrupcion = false;
        pthread_mutex_unlock(&mutex_interrupt);

        break;  // Se corta el ciclo
    }

    if (contexto->program_counter == -1) break;

    destruir_instruccion_decodificada(instruccion); // libero mallocs
}
    destruir_estructuras_del_contexto_actual(contexto);
}


t_contexto* recibir_contexto(int fd) {
    t_contexto* contexto = malloc(sizeof(t_contexto));
    
    recv(fd, &(contexto->pid), sizeof(uint32_t), 0);
    recv(fd, &(contexto->program_counter), sizeof(uint32_t), 0);
    recv(fd, &(contexto->AX), sizeof(uint32_t), 0);
    recv(fd, &(contexto->BX), sizeof(uint32_t), 0);
    recv(fd, &(contexto->CX), sizeof(uint32_t), 0);
    recv(fd, &(contexto->DX), sizeof(uint32_t), 0);

    return contexto;
}

void* escuchar_interrupt(void* arg) {
    int fd = *(int*)arg;

    while (1) {
        op_code codigo;
        if (recv(fd, &codigo, sizeof(op_code), 0) > 0) {
            if (codigo == MENSAJE) {  
                pthread_mutex_lock(&mutex_interrupt);
                flag_interrupcion = true;
                pthread_mutex_unlock(&mutex_interrupt);

                log_info(logger, "Llega interrupción al puerto Interrupt");
            }
        }
    }
    return NULL;
}

bool hay_interrupcion() {
    pthread_mutex_lock(&mutex_interrupt);
    bool resultado = flag_interrupcion;
    pthread_mutex_unlock(&mutex_interrupt);
    return resultado;
}
////////////////////////////////////////////////////////////////////   FETCH   ///////////////////////////////////////////////////////////////////////////////////
char* ciclo_de_instruccion_fetch(int conexion_memoria, t_contexto* contexto) {
    log_info(logger, "## PID: %d - FETCH - Program Counter: %d", contexto->pid, contexto->program_counter);

    // ENVIO A MEMORIA EL PEDIDO DE INSTRUCCION
    t_paquete* paquete = crear_paquete(PEDIR_INSTRUCCION);
    agregar_a_paquete(paquete, &(contexto->pid), sizeof(uint32_t));
    agregar_a_paquete(paquete, &(contexto->program_counter), sizeof(uint32_t));
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);

    // RECIBO LA RTA AL PEDIDO
    char* instruccion = recibir_instruccion(conexion_memoria); 
    log_info(logger, "Instrucción recibida: %s", instruccion);
    return instruccion;
}

char* recibir_instruccion(int socket_memoria) {
    op_code codigo_operacion;
    if (recv(socket_memoria, &codigo_operacion, sizeof(op_code), 0) <= 0) {
        return NULL;
    }

    if (codigo_operacion != DEVOLVER_INSTRUCCION) {
        return NULL; // O manejar error de tipo inesperado
    }

    // Recibo la instrucción COMO STRING
    uint32_t size;
    recv(socket_memoria, &size, sizeof(uint32_t), 0);

    char* instruccion = malloc(size);
    recv(socket_memoria, instruccion, size, 0);
    return instruccion;
}
//////////////////////////////////////////////////////////////////////   DECODE   ///////////////////////////////////////////////////////////////////////////////////
t_instruccion_decodificada* ciclo_de_instruccion_decode(char* instruccion_cruda) {
    return decodificar_instruccion(instruccion_cruda);
}

t_instruccion_decodificada* decodificar_instruccion(char* instruccion_cruda) {
    char** tokens = string_split(instruccion_cruda, " ");
    if (!tokens) return NULL;

    t_instruccion_decodificada* instruccion = malloc(sizeof(t_instruccion_decodificada));
    instruccion->opcode = strdup(tokens[0]);

    // Cuento cantidad de operandos
    int cantidad = 0;
    for (int i = 1; tokens[i] != NULL; i++) cantidad++;

    instruccion->cantidad_operandos = cantidad;
    instruccion->operandos = malloc(sizeof(char*) * cantidad);

    for (int i = 0; i < cantidad; i++) {
        instruccion->operandos[i] = strdup(tokens[i + 1]);
    }

    // Para ver si requiere traducción de dirección
    if (string_equals_ignore_case(instruccion->opcode, "READ") ||
        string_equals_ignore_case(instruccion->opcode, "WRITE")) {
        instruccion->necesita_traduccion = true;
    } else {
        instruccion->necesita_traduccion = false;
    }

    string_array_destroy(tokens);
    return instruccion;
}

//////////////////////////////////////////////////////////////EXECUTE///////////////////////////////////////////////////////
void ciclo_de_instruccion_execute(t_instruccion_decodificada* instruccion, t_contexto* contexto, t_log* logger, int conexion_memoria) {
    char* opcode = instruccion->opcode;

    log_info(logger, "## PID: %d - Ejecutando: %s", contexto->pid, opcode);

    if (string_equals_ignore_case(opcode, "NOOP")) {
        usleep(1000 * 1000); // (1 seg)
    }
    else if (string_equals_ignore_case(opcode, "GOTO")) {
        if (instruccion->cantidad_operandos >= 1) {
            int nuevo_pc = atoi(instruccion->operandos[0]);
            contexto->program_counter = nuevo_pc;
        } else {
            log_error(logger, "GOTO sin operando");
        }
    }
    else if (string_equals_ignore_case(opcode, "EXIT")) {
        contexto->program_counter = -1;
    }
    else if (string_equals_ignore_case(opcode, "IO")) {
    int id_dispositivo = atoi(instruccion->operandos[0]);
    int tiempo = atoi(instruccion->operandos[1]);

    log_info(logger, "PID: %d - Ejecutando: IO - Dispositivo: %d - Tiempo: %d", contexto->pid, id_dispositivo, tiempo);

    enviar_contexto_a_kernel_io(contexto, CAUSA_IO, conexion_kernel_dispatch, logger, id_dispositivo, tiempo);
    contexto->program_counter = -1;
}

else if (string_equals_ignore_case(opcode, "INIT_PROC")) {
    char* archivo_instrucciones = instruccion->operandos[0];
    int tamanio = atoi(instruccion->operandos[1]);

    log_info(logger, "PID: %d - Ejecutando: INIT_PROC - Archivo: %s - Tamaño: %d", contexto->pid, archivo_instrucciones, tamanio);

    enviar_contexto_a_kernel_init_proc(contexto, INIT_PROC, conexion_kernel_dispatch, logger, archivo_instrucciones, tamanio);
    contexto->program_counter = -1;
}


else if (string_equals_ignore_case(opcode, "DUMP_MEMORY")) {
    log_info(logger, "PID: %d - Ejecutando: DUMP_MEMORY", contexto->pid);
    enviar_contexto_a_kernel(contexto, SIGNAL, conexion_kernel_dispatch, logger); // SEGUNDO PARAMETRO ES EL MOTIVO DE DESLOJO (PUSE SIGNAL XQ SI)
    contexto->program_counter = -1;
}

    else if (string_equals_ignore_case(opcode, "READ")) {
    uint32_t direccion_logica = atoi(instruccion->operandos[0]); //el primer parametro de READ es la dir logica, el segundo el tamanio
    uint32_t tamanio = atoi(instruccion->operandos[1]);
    uint32_t direccion_fisica = traducir_direccion_logica(direccion_logica, 256, contexto, conexion_memoria); 

    log_info(logger, "PID: %d - Acción: LEER - Dirección Física: %d", contexto->pid, direccion_fisica);

    t_paquete* paquete = crear_paquete(LEER_MEMORIA);
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(uint32_t));
    agregar_a_paquete(paquete, &tamanio, sizeof(uint32_t));
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);

    // Recibir valor leído
    uint32_t valor;
    recv(conexion_memoria, &valor, sizeof(uint32_t), 0);

    log_info(logger, "Valor leído: %d", valor);
}
else if (string_equals_ignore_case(opcode, "WRITE")) {
    uint32_t direccion_logica = atoi(instruccion->operandos[0]);  //el primer parametro de WRITE es la dir logica, el segundo son los datos
    char* valor = instruccion->operandos[1];

    uint32_t direccion_fisica = traducir_direccion_logica(direccion_logica, 256, contexto, conexion_memoria);

    log_info(logger, "PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %s", contexto->pid, direccion_fisica, valor);

    t_paquete* paquete = crear_paquete(ESCRIBIR_MEMORIA);
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(uint32_t));
    agregar_a_paquete(paquete, valor, strlen(valor) + 1);
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);
}

    // Si la instrucción no fue GOTO (que cambia el PC)y avanza1 para buscar una instruccion a ejecutar
    else {
        contexto->program_counter++;
}

}

void enviar_contexto_a_kernel(t_contexto* contexto, motivo_desalojo motivo, int fd, t_log* logger) {
    char* motivo_str[] = {
    "EXIT", "CAUSA_IO", "WAIT", "SIGNAL", "PAGE_FAULT", "INTERRUPCION", "DESALOJO_POR_QUANTUM"}; //mismo orden que el enum de motivodesalojo
log_info(logger, "## PID: %d - Desalojado - Motivo: %s", contexto->pid, motivo_str[motivo]);

    t_paquete* paquete = crear_paquete(CONTEXTO_PROCESO); // contxtoproceso es opcode

    agregar_a_paquete(paquete, &(contexto->pid), sizeof(uint32_t));
    agregar_a_paquete(paquete, &(contexto->program_counter), sizeof(uint32_t));
    agregar_a_paquete(paquete, &(contexto->AX), sizeof(uint32_t));
    agregar_a_paquete(paquete, &(contexto->BX), sizeof(uint32_t));
    agregar_a_paquete(paquete, &(contexto->CX), sizeof(uint32_t));
    agregar_a_paquete(paquete, &(contexto->DX), sizeof(uint32_t));
    agregar_a_paquete(paquete, &motivo, sizeof(motivo_desalojo));  
    enviar_paquete(paquete, fd);
    eliminar_paquete(paquete);
}

void enviar_contexto_a_kernel_init_proc(t_contexto* contexto, motivo_desalojo motivo, int fd, t_log* logger, char* archivo, int tamanio) {
    t_paquete* paquete = crear_paquete(DEVOLVER_CONTEXTO);

    agregar_a_paquete(paquete, &(contexto->pid), sizeof(uint32_t));
    agregar_a_paquete(paquete, &(contexto->program_counter), sizeof(uint32_t));
    agregar_a_paquete(paquete, &(contexto->AX), sizeof(uint32_t));
    agregar_a_paquete(paquete, &(contexto->BX), sizeof(uint32_t));
    agregar_a_paquete(paquete, &(contexto->CX), sizeof(uint32_t));
    agregar_a_paquete(paquete, &(contexto->DX), sizeof(uint32_t));
    agregar_a_paquete(paquete, &motivo, sizeof(int));
    agregar_a_paquete(paquete, &tamanio, sizeof(int));
    agregar_a_paquete(paquete, archivo, strlen(archivo) + 1); // string con '\0'

    enviar_paquete(paquete, fd);
    eliminar_paquete(paquete);

    log_info(logger, "Se envio el contexto actualizado con motivo INIT_PROC al Kernel.");
}

void enviar_contexto_a_kernel_io(t_contexto* contexto, motivo_desalojo motivo, int fd, t_log* logger, int id_io, int tiempo_io) {
    t_paquete* paquete = crear_paquete(DEVOLVER_CONTEXTO);

    agregar_a_paquete(paquete, &(contexto->pid), sizeof(uint32_t));
    agregar_a_paquete(paquete, &(contexto->program_counter), sizeof(uint32_t));
    agregar_a_paquete(paquete, &(contexto->AX), sizeof(uint32_t));
    agregar_a_paquete(paquete, &(contexto->BX), sizeof(uint32_t));
    agregar_a_paquete(paquete, &(contexto->CX), sizeof(uint32_t));
    agregar_a_paquete(paquete, &(contexto->DX), sizeof(uint32_t));
    agregar_a_paquete(paquete, &motivo, sizeof(int));
    agregar_a_paquete(paquete, &id_io, sizeof(int));
    agregar_a_paquete(paquete, &tiempo_io, sizeof(int));

    enviar_paquete(paquete, fd);
    eliminar_paquete(paquete);

    log_info(logger, "Se envio el contexto actualizado con motivo IO al Kernel.");
}



//esto va en archivo MMU.C
uint32_t traducir_direccion_logica(uint32_t direccion_logica, int tamanio_pagina, t_contexto* contexto, int conexion_memoria) {
    uint32_t nro_pagina     = direccion_logica / tamanio_pagina;
    uint32_t desplazamiento = direccion_logica % tamanio_pagina;

    // Log obligatorio
    log_info(logger, "PID: %d - OBTENER MARCO - Página: %d", contexto->pid, nro_pagina);

    // Enviar solicitud de marco a Memoria
    t_paquete* paquete = crear_paquete(PEDIR_MARCO);
    agregar_a_paquete(paquete, &(contexto->pid), sizeof(uint32_t));
    agregar_a_paquete(paquete, &nro_pagina, sizeof(uint32_t));
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);

    // Recibir respuesta con num de marco
    uint32_t marco;
    recv(conexion_memoria, &marco, sizeof(uint32_t), 0);

    log_info(logger, "PID: %d - Marco recibido: %d", contexto->pid, marco);

    return marco * tamanio_pagina + desplazamiento;
}

