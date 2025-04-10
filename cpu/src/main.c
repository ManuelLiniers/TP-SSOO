#include <utils/hello.h>
#include <utils/commons.h>
int main(int argc, char* argv[]) {
    saludar("cpu");

    t_log* logger = log_create("cpu.log", "[CPU]", 1, LOG_LEVEL_DEBUG);
    if(logger == NULL){
        perror("error al crear el logger");
        abort();
    } 

    t_config* cpu_config = create_config("/home/utnso/tp-2025-1c-queCompileALaPrimera/cpu/cpu.config");
    if(cpu_config == NULL){
        log_error(logger, "error con el config del cpu");
        abort();
    }


    return 0;
}
