#include <stdio.h>
#include <pthread.h>

// Declaração das variáveis globais que serão usadas pelas threads
long x;
int is_prime = 1; // Assumimos que 1 é primo
pthread_t thread1, thread2;
pthread_mutex_t lock;

// Função para verificar se um número é primo
void *check_prime(void *arg) {
    long start = (long)arg;
    long end = start + x / 2;

    if (end > x - 1) end = x - 1;

    for (long i = start; i <= end; i++) {
        if (i > 1 && x % i == 0) {
            pthread_mutex_lock(&lock); // Usa o mutex para acessar a variável global com segurança
            is_prime = 0; // Número não é primo
            pthread_mutex_unlock(&lock);

            // Cancela a outra thread de forma mais robusta
            pthread_t current_thread = pthread_self();
            if (pthread_equal(current_thread, thread1)) {
                pthread_cancel(thread2);
            } else {
                pthread_cancel(thread1);
            }

            pthread_exit(NULL);
        }
    }
    pthread_exit(NULL);
}

int main(){
    scanf("%ld",&x);

    // Verifica se o número é <= 1 (não é primo)
    if (x <= 1) {
        printf("0\n"); // Não é primo
        return 0;
    }

    // Inicializa o mutex
    pthread_mutex_init(&lock, NULL);

    // Cria as threads, dividindo a verificação em duas faixas
    pthread_create(&thread1, NULL, check_prime, (void *)2);
    pthread_create(&thread2, NULL, check_prime, (void *)(x / 2 + 1));

    // Aguarda as threads terminarem
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    // Imprimi o resultado
    printf("%d\n", is_prime);

    // Destrói o mutex
    pthread_mutex_destroy(&lock);

    return 0;        
}