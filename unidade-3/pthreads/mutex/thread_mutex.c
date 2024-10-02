#include <stdio.h>
#include <pthread.h>

#define NUM_THREADS 128
#define NUM_INCREMENTS 1000

long global_counter = 0;
pthread_mutex_t mutex;

void* increment_counter(void* arg) {
    for (int i = 0; i < NUM_INCREMENTS; i++) {
        pthread_mutex_lock(&mutex);
        global_counter++;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    
    // Inicializa o mutex
    pthread_mutex_init(&mutex, NULL);

    // Cria as threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, increment_counter, NULL);
    }

    // Espera as threads terminarem
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Imprimi o valor final do contador
    printf("Valor final do contador: %ld\n", global_counter);

    // Destrui o mutex
    pthread_mutex_destroy(&mutex);

    return 0;
}
