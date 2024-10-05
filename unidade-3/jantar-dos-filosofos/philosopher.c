#include "philosopher.h"
#include <unistd.h>
#include <time.h>

// Variáveis globais
extern char state[N + 1];          // Vetor de estados dos filósofos
extern pthread_mutex_t mutex;      // Mutex para proteger o vetor de estados
extern sem_t sem_philosopher[N];   // Semáforos para sincronização

// Macros para obter o filósofo esquerdo e direito
#define LEFT(i) ((i + N - 1) % N)
#define RIGHT(i) ((i + 1) % N)

// Função para testar se o filósofo pode comer
void test(int i) {
    if (state[i] == FAMINTO &&
        state[LEFT(i)] != COMENDO &&
        state[RIGHT(i)] != COMENDO) {
        state[i] = COMENDO;
        printf("%s\n", state);
        sem_post(&sem_philosopher[i]);
    }
}

// Função para pegar os garfos (tentar comer)
void take_forks(int i) {
    pthread_mutex_lock(&mutex);
    state[i] = FAMINTO;
    printf("%s\n", state);
    test(i);
    pthread_mutex_unlock(&mutex);
    sem_wait(&sem_philosopher[i]);
}

// Função para devolver os garfos (parar de comer)
void put_forks(int i) {
    pthread_mutex_lock(&mutex);
    state[i] = PENSANDO;
    printf("%s\n", state);
    test(LEFT(i));
    test(RIGHT(i));
    pthread_mutex_unlock(&mutex);
}

// Função principal da thread do filósofo
void* philosopher(void* num) {
    int i = *(int*)num;
    unsigned int seed = time(NULL) ^ i;  // Semente para o rand_r

    while (1) {
        // Pensando
        sleep(rand_r(&seed) % 3 + 1);
        // Tentando comer
        take_forks(i);
        // Comendo
        sleep(rand_r(&seed) % 3 + 1);
        // Terminou de comer
        put_forks(i);
    }
}
