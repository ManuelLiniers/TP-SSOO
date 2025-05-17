#include<semaphore.h>
#include<utils/commons.h>

extern pthread_mutex_t mutex_queue_new;
extern pthread_mutex_t mutex_queue_ready;
extern pthread_mutex_t mutex_queue_block;
extern pthread_mutex_t mutex_queue_exit;

void iniciar_mutex();
void signal_sem(sem_t *sem);
void wait_sem(sem_t *sem);
void signal_mutex(pthread_mutex_t *mutex);
void wait_mutex(pthread_mutex_t *mutex);
