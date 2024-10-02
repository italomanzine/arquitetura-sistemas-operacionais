#include <iostream>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <errno.h>

int main() {
    // Inicializa a semente do gerador de números aleatórios
    srand(time(NULL));

    int nprocs;
    std::cout << "Digite o número de processos a serem criados: ";
    std::cin >> nprocs;

    if (nprocs <= 0) {
        std::cerr << "O número de processos deve ser maior que 0." << std::endl;
        return 1;
    }

    // Cria uma região de memória compartilhada para a variável 'id'
    int *id = (int*) mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, 
                          MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (id == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    *id = 0; // Inicializa 'id' com 0

    // Define o nome do semáforo
    const char* sem_name = "/meu_semaforo_unico";

    // Tenta remover qualquer semáforo existente com o mesmo nome
    sem_unlink(sem_name);

    // Cria e inicializa o semáforo compartilhado
    sem_t *sem = sem_open(sem_name, O_CREAT | O_EXCL, 0600, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        munmap(id, sizeof(int));
        return 1;
    }

    // Cria os processos filhos
    for (int i = 0; i < nprocs; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            // Em caso de erro, continua para tentar criar os demais processos
            continue;
        } else if (pid == 0) {
            // Processo filho

            // Gera um tempo aleatório entre 1 e 10 segundos
            int sleep_time = (rand() % 10) + 1;
            sleep(sleep_time);

            // Entra na região crítica
            if (sem_wait(sem) < 0) {
                perror("sem_wait");
                munmap(id, sizeof(int));
                sem_close(sem);
                exit(1);
            }

            // Incrementa 'id' e imprime a mensagem
            (*id)++;
            std::cout << "Processo " << *id << " criado" << std::endl;

            // Sai da região crítica
            if (sem_post(sem) < 0) {
                perror("sem_post");
                munmap(id, sizeof(int));
                sem_close(sem);
                exit(1);
            }

            // Desaloca recursos no filho
            munmap(id, sizeof(int));
            sem_close(sem);
            exit(0);
        }
        // Processo pai continua o loop para criar mais filhos
    }

    // Processo pai espera todos os filhos terminarem
    for (int i = 0; i < nprocs; ++i) {
        wait(NULL);
    }

    // Limpa os recursos
    sem_close(sem);
    sem_unlink(sem_name);
    munmap(id, sizeof(int));

    std::cout << "Todos os processos filhos foram criados e finalizados." << std::endl;

    return 0;
}
