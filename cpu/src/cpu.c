#include "cpu.h"

int conexion_memoria;
int conexion_kernel_dispatch;
int conexion_kernel_interrupt;
bool flag_interrupcion = false;
int TAMANIO_PAGINA;
int ENTRADAS_CACHE;
int retardo_cache;

int id;

t_log* logger = NULL;
t_config* cpu_config = NULL;
t_config* pruebas_config = NULL;
pthread_mutex_t mutex_interrupt;

int main(int argc, char* argv[]) {
    //saludar("cpu");

    logger = crear_log();

    id = atoi(argv[1]);
    log_debug(logger, "Id CPU: %d", id);
    cpu_config = crear_config(logger, "cpu.config");
    char* ruta_config = malloc(sizeof(argv[1])+sizeof("cpu")+sizeof(argv[2]));
    //char* ruta_config = malloc(sizeof(argv[1])+sizeof("cpu")+sizeof("_largo_plazo.config"));
    strcpy(ruta_config, "cpu");
    strcat(ruta_config, argv[1]);
    //strcat(ruta_config, "_corto_plazo.config");
    strcat(ruta_config, argv[2]);
    //strcat(ruta_config, "_largo_plazo.config");
    pruebas_config = crear_config(logger,ruta_config);
    //free(ruta_config);

    inicializar_tlb(logger, pruebas_config);
    inicializar_cache(logger, pruebas_config);

    TAMANIO_PAGINA = config_get_int_value(pruebas_config, "TAMANIO_PAGINA");
    retardo_cache = config_get_int_value(pruebas_config, "RETARDO_CACHE");

    //log_info(logger, "CPU - TAM_PAGINA = %d", TAMANIO_PAGINA);


    //ENTRADAS_CACHE = config_get_int_value(cpu_config, "ENTRADAS_CACHE");
        
    pthread_mutex_init(&mutex_interrupt, NULL);  
        /*  typedef union
    {
    struct __pthread_mutex_s __data;
    char __size[__SIZEOF_PTHREAD_MUTEX_T];
    long int __align;
    } pthread_mutex_t;   */


    char* ip_memoria = config_get_string_value(cpu_config, "IP_MEMORIA");
	char* puerto_memoria = config_get_string_value(cpu_config, "PUERTO_MEMORIA");

    char* ip_kernel = config_get_string_value(cpu_config, "IP_KERNEL");
	char* puerto_kernel_dispatch = config_get_string_value(cpu_config, "PUERTO_KERNEL_DISPATCH");
	char* puerto_kernel_interrupt = config_get_string_value(cpu_config, "PUERTO_KERNEL_INTERRUPT");

    conexion_memoria = crear_conexion(logger, ip_memoria, puerto_memoria);
    conexion_kernel_dispatch = crear_conexion(logger, ip_kernel, puerto_kernel_dispatch);
    conexion_kernel_interrupt = crear_conexion(logger, ip_kernel, puerto_kernel_interrupt);
    abrir_conexion_kernel(conexion_kernel_dispatch);
    abrir_conexion_kernel(conexion_kernel_interrupt);
    abrir_conexion_memoria(conexion_memoria);
    /* hacer_handshake(logger, conexion_kernel_dispatch);
    enviarCodigo(conexion_kernel_dispatch,HANDSHAKE);
    hacer_handshake(logger, conexion_kernel_interrupt);
    enviarCodigo(conexion_kernel_interrupt,HANDSHAKE);

    abrir_conexion_memoria(ip_memoria, puerto_memoria); */
    

    // mensaje_inicial(conexion_memoria, conexion_kernel_dispatch, conexion_kernel_interrupt);


    pthread_t hilo_interrupt;
    pthread_create(&hilo_interrupt, NULL, escuchar_interrupt, &conexion_kernel_interrupt);  //lanzo funcion para hilo paralelo para que escuche constantemente el socket del puerto de interrupciones
    pthread_detach(hilo_interrupt);

    //espero PCBs del Kernel por dispatch
    while (1) {
        log_debug(logger, "Esperando op_code desde Kernel...");

        op_code cod_op;
        if (recv(conexion_kernel_dispatch, &cod_op, sizeof(op_code), MSG_WAITALL) <= 0) {
            log_error(logger, "Fallo al recibir op_code del Kernel.");
            break;
        }

        if (cod_op == CONTEXTO_PROCESO) {
            log_debug(logger, "Recibido CONTEXTO_PROCESO");
            t_buffer* buffer = recibir_paquete(conexion_kernel_dispatch);
            t_contexto* contexto = deserializar_contexto(buffer);
            atender_proceso_del_kernel(contexto, logger);
            free(buffer->stream);
            free(buffer);
        } else {
            log_error(logger, "Código de operación desconocido: %d", cod_op);
        }
    }
    terminar_programa(conexion_memoria, conexion_kernel_dispatch, conexion_kernel_interrupt, logger, cpu_config);

    return 0;
}

t_buffer* recibir_buffer_contexto(int socket) {
    t_buffer* buffer = malloc(sizeof(t_buffer));

    recv(socket, &(buffer->size), sizeof(int), MSG_WAITALL);
    buffer->stream = malloc(buffer->size);
    recv(socket, buffer->stream, buffer->size, MSG_WAITALL);

    return buffer;
}

t_contexto* deserializar_contexto(t_buffer* buffer) {
    t_contexto* contexto = malloc(sizeof(t_contexto));

    /*int offset = 0;
    memcpy(&(contexto->pid), buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&(contexto->program_counter), buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&(contexto->AX), buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&(contexto->BX), buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&(contexto->CX), buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&(contexto->DX), buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);*/

    contexto->pid = recibir_int_del_buffer(buffer);
    contexto->program_counter = recibir_int_del_buffer(buffer);
    /* Iria de esta manera pero KERNEL no manda los registros
    contexto->AX = recibir_int_del_buffer(buffer);
    contexto->BX = recibir_int_del_buffer(buffer);
    contexto->CX = recibir_int_del_buffer(buffer);
    contexto->DX = recibir_int_del_buffer(buffer);*/

    return contexto;
}


t_log* crear_log(){

    t_log* logger = log_create("cpu.log", "[CPU]", 1, LOG_LEVEL_INFO);
    return logger;
}

t_config* crear_config(t_log* logger, char* archivo){
    // char* resultado = malloc(sizeof("/home/utnso/tp-2025-1c-queCompileALaPrimera/cpu/")+sizeof(archivo)+1);
    // strcpy(resultado, "/home/utnso/tp-2025-1c-queCompileALaPrimera/cpu/");
    // strcpy(resultado, archivo);
    // //resultado = strcat("/home/utnso/tp-2025-1c-queCompileALaPrimera/cpu/",archivo);
    // t_config* cpu_config = config_create(resultado);
    // if(cpu_config == NULL){
    //     log_error(logger, "error con el config del cpu");
    //     abort();
    // }
    // free(resultado);
    char* base_path = "/cpu/";
	char* resultado = malloc(strlen(base_path) + strlen(archivo) + 1);
	strcpy(resultado, base_path);
	strcat(resultado, archivo);

	t_config* config_pruebas = config_create(resultado);
	if (config_pruebas == NULL) {
    	log_error(logger, "config_kernel es NULL");
    	exit(EXIT_FAILURE);
	}	
	free(resultado);
    return config_pruebas;
}

void mensaje_inicial(int conexion_memoria, int conexion_kernel_dispatch, int conexion_kernel_interrupt){
    enviar_mensaje("Conexion inicial de la CPU", conexion_memoria);
    enviar_mensaje("Conexion inicial de la CPU - Dispatch", conexion_kernel_dispatch);
    enviar_mensaje("Conexion inicial de la CPU - Interrumpt", conexion_kernel_interrupt);
}

void terminar_programa(int conexion_memoria, int conexion_kernel_dispatch, int conexion_kernel_interrupt, t_log* logger, t_config* cpu_config)
{
    log_debug(logger, "cliente cpu terminado");
	log_destroy(logger);
    config_destroy(cpu_config);
    close(conexion_memoria);
    close(conexion_kernel_dispatch);
    close(conexion_kernel_interrupt);
}

void atender_proceso_del_kernel(t_contexto* contexto, t_log* logger) {
    log_debug(logger, "Comenzando ejecución del proceso con PID %d", contexto->pid);

    while (1) {
        char* instruccion_cruda = ciclo_de_instruccion_fetch(conexion_memoria, contexto);
        if (!instruccion_cruda) { //si el fetch devuelve NULL entonces haria !NULL y el if vería un 1 entonces entra al bloque
            log_error(logger, "Fallo al recibir la instrucción desde Memoria.");
            break;
        }

        t_instruccion_decodificada* instruccion = ciclo_de_instruccion_decode(instruccion_cruda);
        free(instruccion_cruda);

        if (!instruccion) {
            log_error(logger, "Fallo al decodificar la instrucción.");
            break;
        }

        ciclo_de_instruccion_execute(instruccion, contexto, logger, conexion_memoria);
        if (hay_interrupcion()) {
            
            log_debug(logger, "Se detectó una interrupción luego de ejecutar la instrucción");
            enviar_contexto_a_kernel(contexto, INTERRUPCION, conexion_kernel_dispatch, logger);
            
            //le avisa al kernel que hubo interrupción, desaloja el proceso y resetea el flag a false para poder seguir escuchando futuras interrupciones

            pthread_mutex_lock(&mutex_interrupt);
            flag_interrupcion = false;
            pthread_mutex_unlock(&mutex_interrupt);
            break;
        }

        if (contexto->program_counter == -1){
            break;
        } 
            

        destruir_instruccion_decodificada(instruccion);
    }

    destruir_estructuras_del_contexto_actual(contexto);
}



void* escuchar_interrupt(void* arg) {
    int fd = *(int*)arg;

    while (1) {
        op_code codigo;
        if (recv(fd, &codigo, sizeof(op_code), 0) > 0) {
            if (codigo == INTERRUPCION_CPU) {  
                pthread_mutex_lock(&mutex_interrupt);
                flag_interrupcion = true;  //FALG QUE PROTEJO CON UN MUTEX PARA QUE AMBOS HILOS NO LA USEN A LA VEZ
                pthread_mutex_unlock(&mutex_interrupt);

                log_info(logger, "## Llega interrupción al puerto Interrupt");
            }
        }
    }
    return NULL;
}

bool hay_interrupcion() {
    pthread_mutex_lock(&mutex_interrupt);
    bool resultado = flag_interrupcion;  //definida al ppio, es false por defecto, se pone en true cuadndo hay una interrupcion
    pthread_mutex_unlock(&mutex_interrupt);
    return resultado;
}


void destruir_estructuras_del_contexto_actual(t_contexto* contexto) {
    limpiar_tlb_por_pid(contexto->pid);  
    limpiar_cache_por_pid(contexto->pid, conexion_memoria, logger); 
    free(contexto);
    

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
    log_debug(logger, "Instrucción recibida: %s", instruccion);
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

//////////////////////////////////////////////////////////////EXECUTE///////////////////////////////////////////////////////
void ciclo_de_instruccion_execute(t_instruccion_decodificada* instruccion, t_contexto* contexto, t_log* logger, int conexion_memoria) {
    char* opcode = instruccion->opcode;

    log_info(logger, "## PID: %d - Ejecutando: %s", contexto->pid, opcode);

    if (string_equals_ignore_case(opcode, "NOOP")) {
        log_info(logger, "PID: %d - Ejecutando: NOOP", contexto->pid);
        contexto->program_counter++;
        return;
    }
    else if (string_equals_ignore_case(opcode, "GOTO")) {
        if (instruccion->cantidad_operandos >= 1) {
            int nuevo_pc = atoi(instruccion->operandos[0]);
            contexto->program_counter = nuevo_pc;
            return;   //sale de la funcion???
        } else {
            log_error(logger, "GOTO sin operando");
        }
    }
    else if (string_equals_ignore_case(opcode, "EXIT")) {
        log_info(logger, "PID: %d - Ejecutando: EXIT", contexto->pid);

        enviar_contexto_a_kernel(contexto, FINALIZADO, conexion_kernel_dispatch, logger); //motivo para que finalice (kernel lo pasa a terminado)
        contexto->program_counter = -1;
    }

    else if (string_equals_ignore_case(opcode, "IO")) {
        char* id_dispositivo = instruccion->operandos[0];
        int tiempo = atoi(instruccion->operandos[1]);

        log_info(logger, "PID: %d - Ejecutando: IO - Dispositivo: %s - Tiempo: %d", contexto->pid, id_dispositivo, tiempo);

        enviar_contexto_a_kernel_io(contexto, CAUSA_IO, conexion_kernel_dispatch, logger, id_dispositivo, tiempo);
        contexto->program_counter = -1;  //desalojo al proceso
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

        // envio la petición de Dump a Memoria
/*        t_paquete* paquete = crear_paquete(DUMP_MEMORY);
        agregar_a_paquete(paquete, &(contexto->pid), sizeof(uint32_t));
        enviar_paquete(paquete, conexion_memoria);
        eliminar_paquete(paquete);*/

        enviar_contexto_a_kernel_dump(contexto, MEMORY_DUMP, conexion_kernel_dispatch, logger);

        // espero respuesta de Memoria
        //int resultado_dump;
        //recv(conexion_memoria, &resultado_dump, sizeof(int), 0);

        //if (resultado_dump == OK) {
          //  enviar_contexto_a_kernel(contexto, SIGNAL, conexion_kernel_dispatch, logger);
        //} else {
        //    enviar_contexto_a_kernel(contexto, FINALIZADO, conexion_kernel_dispatch, logger);
        //}

        contexto->program_counter = -1; // fin del proceso en CPU
    }  

    else if (string_equals_ignore_case(opcode, "READ")) {
    uint32_t direccion_logica = atoi(instruccion->operandos[0]);
    uint32_t tamanio = atoi(instruccion->operandos[1]);
    int pid = contexto->pid;

    uint32_t nro_pagina = direccion_logica / TAMANIO_PAGINA;
    uint32_t desplazamiento = direccion_logica % TAMANIO_PAGINA;
    uint32_t direccion_fisica = 0;

    char* lectura = malloc(TAMANIO_PAGINA); 
    char* contenido_leido = NULL;


    // Primero busco en cache
    if(entradas_cache > 0){
        usleep(retardo_cache * 1000);    //acceso a cache
        contenido_leido = buscar_contenido_cache(pid, floor(direccion_logica/TAMANIO_PAGINA), false); // leo desde el desplazamiento hasta el tam de la pagina
    }
    if(contenido_leido != NULL){
        memcpy(lectura, contenido_leido + desplazamiento, tamanio);
        log_info(logger, "PID: %d - Acción: LEER - Valor: %s", pid, lectura);
        contexto->program_counter++;
        return;
    }
    
    
    // Luego busco en TLB
    uint32_t marco = -1;
    bool en_TLB = false;
    if (buscar_en_tlb(contexto->pid, nro_pagina, &marco, logger)) {
        direccion_fisica = marco * TAMANIO_PAGINA + desplazamiento;
        en_TLB = true;
    }
    if(marco == -1){
        log_debug(logger, "PID: %d - OBTENER MARCO - Página: %d", contexto->pid, nro_pagina);

        //SI LA PAGINA NO ESTÁ NI EN LAS ENTRADAS DE CACHE NI EN LAS DE TLB ENTONCES BUSCA DIRECTO EN MEMORIA (HUBO DOBLE MISS)

        t_paquete* paquete = crear_paquete(PEDIR_MARCO); //segun el issue 4702: "Pueden enviar un solo mensaje desde CPU con PID y nro de página, y eso representa los N accesos a memoria
        agregar_a_paquete(paquete, &(contexto->pid), sizeof(uint32_t));  
        agregar_a_paquete(paquete, &nro_pagina, sizeof(uint32_t));
        enviar_paquete(paquete, conexion_memoria);
        eliminar_paquete(paquete);

        recv(conexion_memoria, &marco, sizeof(uint32_t), 0);  //recibo marco de la memoria
        log_info(logger, "PID: %d - OBTENER MARCO - Página: %d - Marco recibido: %d", contexto->pid, nro_pagina, marco);
        direccion_fisica = marco * TAMANIO_PAGINA + desplazamiento;
    }

    t_paquete* paquete_contenido = crear_paquete(LEER_MEMORIA);   //pido contenido a memoria 
    agregar_a_paquete(paquete_contenido, &(contexto->pid), sizeof(uint32_t));
    uint32_t dir_fisica = marco * TAMANIO_PAGINA;
    agregar_a_paquete(paquete_contenido, &dir_fisica, sizeof(uint32_t)); // le paso el marco asociado a la pagina que tiene el contenido q pido
    agregar_a_paquete(paquete_contenido, &TAMANIO_PAGINA, sizeof(int));
    enviar_paquete(paquete_contenido, conexion_memoria);
    eliminar_paquete(paquete_contenido);

    //uint32_t tamanio_contenido;  //creo variable del tamaño del contenido
    //recv(conexion_memoria, &tamanio_contenido, sizeof(uint32_t), 0);
    char* contenido_real = malloc(TAMANIO_PAGINA);
    recv(conexion_memoria, contenido_real, TAMANIO_PAGINA, 0);

    if (entradas_cache > 0) {
        agregar_a_cache(contexto->pid, nro_pagina, contenido_real, marco, logger, conexion_memoria, false); 
    }
    if (entradas_tlb > 0 && !en_TLB) {
        agregar_a_tlb(contexto->pid, nro_pagina, marco, logger);
    }

    contexto->program_counter++;

    memcpy(lectura, contenido_real + desplazamiento, tamanio);
    log_info(logger, "PID: %d - Acción: LEER - Dirección Física: %d - Valor: %s", pid, direccion_fisica, lectura);
    log_debug(logger, "Valor leído: %.*s", tamanio, contenido_real);

    free(lectura);
}

   else if (string_equals_ignore_case(opcode, "WRITE")) {
    uint32_t direccion_logica = atoi(instruccion->operandos[0]);
    char* valor = instruccion->operandos[1];
    int tamanio = strlen(valor);

    uint32_t nro_pagina = direccion_logica / TAMANIO_PAGINA;
    uint32_t desplazamiento = direccion_logica % TAMANIO_PAGINA;
    uint32_t direccion_fisica = 0;

    char* contenido_cache = NULL;
    // Primero busco en cache
    if(entradas_cache > 0){
        usleep(retardo_cache * 1000);    //acceso a cache
        contenido_cache = buscar_contenido_cache(contexto->pid, floor(direccion_logica/TAMANIO_PAGINA), true); // leo desde el desplazamiento hasta el tam de la pagina
    }
    if(contenido_cache != NULL){
        log_info(logger, "PID: %d - Acción: ESCRIBIR - Valor: %s", contexto->pid, valor);
        memcpy(contenido_cache + desplazamiento, valor, tamanio);
        contexto->program_counter++;
        return;
    }
    
    
    // Luego busco en TLB
    uint32_t marco = -1;
    bool en_TLB = false;
    if (buscar_en_tlb(contexto->pid, nro_pagina, &marco, logger)) {
        direccion_fisica = marco * TAMANIO_PAGINA + desplazamiento;
        en_TLB = true;
    }
    if(marco == -1){
        log_debug(logger, "PID: %d - OBTENER MARCO - Página: %d", contexto->pid, nro_pagina);

        //SI LA PAGINA NO ESTÁ NI EN LAS ENTRADAS DE CACHE NI EN LAS DE TLB ENTONCES BUSCA DIRECTO EN MEMORIA (HUBO DOBLE MISS)

        t_paquete* paquete = crear_paquete(PEDIR_MARCO); //segun el issue 4702: "Pueden enviar un solo mensaje desde CPU con PID y nro de página, y eso representa los N accesos a memoria
        agregar_a_paquete(paquete, &(contexto->pid), sizeof(uint32_t));  
        agregar_a_paquete(paquete, &nro_pagina, sizeof(uint32_t));
        enviar_paquete(paquete, conexion_memoria);
        eliminar_paquete(paquete);

        recv(conexion_memoria, &marco, sizeof(uint32_t), 0);  //recibo marco de la memoria
        log_info(logger, "PID: %d - OBTENER MARCO - Página: %d - Marco recibido: %d", contexto->pid, nro_pagina, marco);
        direccion_fisica = marco * TAMANIO_PAGINA + desplazamiento;
    }

    if (entradas_cache > 0) {
        t_paquete* paquete_contenido = crear_paquete(LEER_MEMORIA);   //pido contenido a memoria 
        agregar_a_paquete(paquete_contenido, &(contexto->pid), sizeof(uint32_t));
        uint32_t dir_fisica = marco * TAMANIO_PAGINA;
        agregar_a_paquete(paquete_contenido, &dir_fisica, sizeof(uint32_t)); // le paso el marco asociado a la pagina que tiene el contenido q pido
        agregar_a_paquete(paquete_contenido, &TAMANIO_PAGINA, sizeof(int));
        enviar_paquete(paquete_contenido, conexion_memoria);
        eliminar_paquete(paquete_contenido);

        char* contenido_real = malloc(TAMANIO_PAGINA);
        recv(conexion_memoria, contenido_real, TAMANIO_PAGINA, 0);

        memcpy(contenido_real + desplazamiento, valor, tamanio);

        agregar_a_cache(contexto->pid, nro_pagina, contenido_real, marco, logger, conexion_memoria, true); 

        if (entradas_tlb > 0 && !en_TLB) {
            agregar_a_tlb(contexto->pid, nro_pagina, marco, logger);
        }
        log_info(logger, "PID: %d - Acción: ESCRIBIR - Valor: %s", contexto->pid, valor);

        contexto->program_counter++;
        return;
    }

    if (entradas_tlb > 0 && !en_TLB) {
        agregar_a_tlb(contexto->pid, nro_pagina, marco, logger);
    }

    t_paquete* paquete = crear_paquete(ESCRIBIR_MEMORIA);
    agregar_a_paquete(paquete, &contexto->pid, sizeof(int));
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(uint32_t));
    agregar_a_paquete(paquete, &tamanio, sizeof(int));
    agregar_a_paquete(paquete, valor, tamanio);
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);

    int respuesta;
    recv(conexion_memoria, &respuesta, sizeof(int), 0);
    if (respuesta != OK) {
        log_error(logger, "No se escribió bien");
    }
    contexto->program_counter++;

    log_info(logger, "PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %s", contexto->pid, direccion_fisica, valor);
    log_debug(logger, "Valor leído: %.*s", tamanio, valor);
    }


    // Si la instrucción no fue GOTO (que cambia el PC),ninguna o alguna que no haya modificado el pc avanza1 para buscar una instruccion a ejecutar


}

void enviar_contexto_a_kernel(t_contexto* contexto, motivo_desalojo motivo, int fd, t_log* logger) {
    char* motivo_str[] = {
    "EXIT", "CAUSA_IO", "WAIT", "SIGNAL", "PAGE_FAULT", "INTERRUPCION", "DESALOJO_POR_QUANTUM"}; //mismo orden que el enum de motivodesalojo
    log_debug(logger, "## PID: %d - Desalojado - Motivo: %s", contexto->pid, motivo_str[motivo]);

    if(motivo != INTERRUPCION){
        contexto->program_counter++;
    }

    t_paquete* paquete = crear_paquete(CONTEXTO_PROCESO); // contxtoproceso es opcode

    agregar_a_paquete(paquete, &(contexto->pid), sizeof(uint32_t));
    agregar_a_paquete(paquete, &(contexto->program_counter), sizeof(uint32_t));
    agregar_a_paquete(paquete, &motivo, sizeof(motivo_desalojo));  
    enviar_paquete(paquete, fd);
    eliminar_paquete(paquete);

    int resultado;
    recv(fd, &resultado, sizeof(int), 0);
    if(resultado != OK){
        log_error(logger, "Contexto Mal recibido por Kernel");
    }
}

void enviar_contexto_a_kernel_init_proc(t_contexto* contexto, motivo_desalojo motivo, int fd, t_log* logger, char* archivo, int tamanio) {
    t_paquete* paquete = crear_paquete(CONTEXTO_PROCESO);

    contexto->program_counter++;

    agregar_a_paquete(paquete, &(contexto->pid), sizeof(uint32_t));
    agregar_a_paquete(paquete, &(contexto->program_counter), sizeof(uint32_t));
    agregar_a_paquete(paquete, &motivo, sizeof(int));
    agregar_a_paquete(paquete, &tamanio, sizeof(int));
    int longitud = strlen(archivo) + 1;
    agregar_a_paquete(paquete, &longitud, sizeof(int));
    agregar_a_paquete(paquete, archivo, strlen(archivo) + 1); // string con '\0'

    enviar_paquete(paquete, fd);
    eliminar_paquete(paquete);

    log_debug(logger, "Se envio el contexto actualizado con motivo INIT_PROC al Kernel.");
    int resultado;
    recv(fd, &resultado, sizeof(int), 0);
    if(resultado != OK){
        log_error(logger, "Contexto Mal recibido por Kernel");
    }
}

void enviar_contexto_a_kernel_io(t_contexto* contexto, motivo_desalojo motivo, int fd, t_log* logger, char* nombre_io, int tiempo_io) {
    t_paquete* paquete = crear_paquete(CONTEXTO_PROCESO);

    contexto->program_counter++;

    agregar_a_paquete(paquete, &(contexto->pid), sizeof(uint32_t));
    agregar_a_paquete(paquete, &(contexto->program_counter), sizeof(uint32_t));
    agregar_a_paquete(paquete, &motivo, sizeof(int));
    int longitud = strlen(nombre_io) + 1;
    agregar_a_paquete(paquete, &longitud, sizeof(int));
    agregar_a_paquete(paquete, nombre_io, strlen(nombre_io) + 1);
    agregar_a_paquete(paquete, &tiempo_io, sizeof(int));

    enviar_paquete(paquete, fd);
    eliminar_paquete(paquete);

    log_debug(logger, "Se envio el contexto actualizado con motivo IO al Kernel.");
    int resultado;
    recv(fd, &resultado, sizeof(int), 0);
    log_debug(logger, "Resultado del contexto proceso recibido");
    if(resultado != OK){
        log_error(logger, "Contexto Mal recibido por Kernel");
    }
}

void enviar_contexto_a_kernel_dump(t_contexto* contexto, motivo_desalojo motivo, int fd, t_log* logger) {
    t_paquete* paquete = crear_paquete(CONTEXTO_PROCESO);

    contexto->program_counter++;

    agregar_a_paquete(paquete, &(contexto->pid), sizeof(uint32_t));
    agregar_a_paquete(paquete, &(contexto->program_counter), sizeof(uint32_t));
    agregar_a_paquete(paquete, &motivo, sizeof(int));

    enviar_paquete(paquete, fd);
    eliminar_paquete(paquete);

    log_debug(logger, "Se envio el contexto actualizado con motivo DUMP al Kernel.");
    int resultado;
    recv(fd, &resultado, sizeof(int), 0);
    if(resultado != OK){
        log_error(logger, "Contexto Mal recibido por Kernel");
    }
}


//CPU no traduce la página en todos los niveles (siendo paginacion multinibvel),
// sino que le delega esa lógica a Memoria. Y Memoria, a partir del PID y la página, accede a las tablas de páginas y responde con el marco correspondiente.
//La idea es reflejar los N accesos en los retardos, y entender la ventaja de usar TLB y caché


void abrir_conexion_memoria(int conexion){
    //conexion_memoria = crear_conexion(logger, ip_memoria, puerto_memoria);

    hacer_handshake(logger, conexion);
    t_paquete* paqueteID = crear_paquete(IDENTIFICACION);

	void* coso_a_enviar = malloc(sizeof(int));
	int codigo = CPU;
	memcpy(coso_a_enviar, &codigo, sizeof(int));
    agregar_a_paquete(paqueteID, coso_a_enviar, sizeof(op_code));
    free(coso_a_enviar);
    enviar_paquete(paqueteID, conexion);
    eliminar_paquete(paqueteID);
}

void abrir_conexion_kernel(int conexion){
    hacer_handshake(logger, conexion);
    t_paquete* paqueteID = crear_paquete(IDENTIFICACION);

    agregar_a_paquete(paqueteID, &id, sizeof(int));
    enviar_paquete(paqueteID, conexion);
    eliminar_paquete(paqueteID);
}
