#include "philosopher.h"
#include <time.h>

// Variáveis globais
pthread_mutex_t mutex;
sem_t sem_philosopher[N];
char state[N + 1];  // Vetor de estados com espaço para o terminador nulo

int main() {
    int i;
    pthread_t thread_id[N];
    int phil_num[N];

    // Inicializa o mutex e os semáforos
    pthread_mutex_init(&mutex, NULL);

    for (i = 0; i < N; i++) {
        sem_init(&sem_philosopher[i], 0, 0);
        state[i] = PENSANDO;
    }
    state[N] = '\0';  // Adiciona o terminador nulo ao final da string

    // Semente para o gerador de números aleatórios
    srand(time(NULL));

    // Imprime o estado inicial
    printf("%s\n", state);

    // Cria as threads dos filósofos
    for (i = 0; i < N; i++) {
        phil_num[i] = i;
        pthread_create(&thread_id[i], NULL, philosopher, &phil_num[i]);
    }

    // Aguarda as threads (o programa roda indefinidamente)
    for (i = 0; i < N; i++) {
        pthread_join(thread_id[i], NULL);
    }

    // Destrói o mutex e os semáforos
    pthread_mutex_destroy(&mutex);
    for (i = 0; i < N; i++) {
        sem_destroy(&sem_philosopher[i]);
    }

    return 0;
}
