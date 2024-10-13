#include <iostream>
#include <unistd.h>    
#include <sys/types.h> 
#include <sys/wait.h>  
#include <cstdlib>     
#include <ctime>       

int main() {
    const int NUM_FILHOS = 10;
    pid_t pids[NUM_FILHOS];
    int pipefd[2];

    // Cria o pipe
    if (pipe(pipefd) == -1) {
        std::perror("pipe");
        return 1;
    }

    // Cria os processos filhos
    for (int i = 0; i < NUM_FILHOS; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            std::perror("fork");
            return 1;
        }
        if (pid == 0) { // Processo filho
            // Fecha o descritor de escrita no filho
            close(pipefd[1]);

            pid_t sorteado;
            ssize_t bytes_lidos = read(pipefd[0], &sorteado, sizeof(sorteado));
            if (bytes_lidos == -1) {
                std::perror("read");
                close(pipefd[0]);
                exit(1);
            } else if (bytes_lidos == 0) {
                // Pipe fechado sem dados
                close(pipefd[0]);
                exit(0);
            }

            // Verifica se o PID lido é o próprio
            if (sorteado == getpid()) {
                std::cout << getpid() << ": fui sorteado" << std::endl;
            }

            // Fecha o descritor de leitura e termina
            close(pipefd[0]);
            exit(0);
        } else { // Processo pai
            pids[i] = pid;
            // Continua o loop para criar todos os filhos
        }
    }

    // Processo pai continua aqui
    // Fecha o descritor de leitura no pai
    close(pipefd[0]);

    // Sorteia um PID aleatório dentre os filhos
    srand(static_cast<unsigned int>(time(nullptr)));
    int indice_sorteado = rand() % NUM_FILHOS;
    pid_t pid_sorteado = pids[indice_sorteado];

    // Imprime o PID sorteado
    std::cout << "PID sorteado: " << pid_sorteado << std::endl;

    // Escreve o PID sorteado 10 vezes no pipe
    for (int i = 0; i < NUM_FILHOS; ++i) {
        ssize_t bytes_escritos = write(pipefd[1], &pid_sorteado, sizeof(pid_sorteado));
        if (bytes_escritos == -1) {
            std::perror("write");
            close(pipefd[1]);
            return 1;
        }
    }

    // Fecha o descritor de escrita no pai
    close(pipefd[1]);

    // Espera que todos os filhos terminem
    for (int i = 0; i < NUM_FILHOS; ++i) {
        waitpid(pids[i], nullptr, 0);
    }

    return 0;
}
