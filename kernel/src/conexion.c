#include "conexion.h"


int crear_conexion_memoria(){
    return crear_conexion(logger_kernel, ip_memoria, puerto_memoria);
}