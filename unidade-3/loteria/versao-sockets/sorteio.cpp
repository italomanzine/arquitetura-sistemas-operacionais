#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <cstring>

#define PORTA 12345
#define NUM_FILHOS 10
#define BUFFER_SIZE 1024

int main() {
    pid_t pid;
    int server_fd, novo_socket;
    struct sockaddr_in endereco;
    int opt = 1;
    int addrlen = sizeof(endereco);
    std::vector<pid_t> pids_filhos;
    std::vector<int> sockets_clientes;

    // Cria socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Falha ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configura opções do socket para reutilização do endereço
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Vincula socket ao endereço e porta
    endereco.sin_family = AF_INET;
    endereco.sin_addr.s_addr = INADDR_ANY;
    endereco.sin_port = htons(PORTA);

    if (bind(server_fd, (struct sockaddr *)&endereco, sizeof(endereco)) < 0) {
        perror("Falha no bind");
        exit(EXIT_FAILURE);
    }

    // Ouvi por conexões
    if (listen(server_fd, NUM_FILHOS) < 0) {
        perror("Falha no listen");
        exit(EXIT_FAILURE);
    }

    // Cria processos filhos
    for (int i = 0; i < NUM_FILHOS; ++i) {
        pid = fork();
        if (pid < 0) {
            perror("Falha no fork");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            // Código do processo filho
            // Cria socket no filho
            int sock = 0;
            struct sockaddr_in serv_addr;

            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("Erro ao criar socket no filho");
                exit(EXIT_FAILURE);
            }

            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(PORTA);

            // Converte endereço IPv4 de texto para binário
            if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
                perror("Endereço inválido/Não suportado no filho");
                exit(EXIT_FAILURE);
            }

            // Conecta ao servidor (pai)
            // Tenta conectar até que o pai esteja pronto
            while (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                usleep(100000); // Espera 100ms antes de tentar novamente
            }

            // Envia PID ao pai
            pid_t pid_filho = getpid();
            if (send(sock, &pid_filho, sizeof(pid_filho), 0) != sizeof(pid_filho)) {
                perror("Falha ao enviar PID do filho");
                close(sock);
                exit(EXIT_FAILURE);
            }

            // Recebe PID sorteado
            pid_t pid_sorteado;
            ssize_t bytes_recebidos = read(sock, &pid_sorteado, sizeof(pid_sorteado));
            if (bytes_recebidos != sizeof(pid_sorteado)) {
                perror("Falha ao receber PID sorteado no filho");
                close(sock);
                exit(EXIT_FAILURE);
            }

            // Verifica se foi sorteado
            if (pid_filho == pid_sorteado) {
                std::cout << pid_filho << ": fui sorteado" << std::endl;
            }

            close(sock);
            exit(EXIT_SUCCESS); // Encerra o processo filho
        }
        // Processo pai continua para criar o próximo filho
    }

    // Processo pai: aceitar conexões e receber PIDs
    for (int i = 0; i < NUM_FILHOS; ++i) {
        if ((novo_socket = accept(server_fd, (struct sockaddr *)&endereco, (socklen_t*)&addrlen)) < 0) {
            perror("Falha no accept");
            exit(EXIT_FAILURE);
        }
        sockets_clientes.push_back(novo_socket);

        // Recebe PID
        pid_t pid_recebido;
        ssize_t bytes_recebidos = read(novo_socket, &pid_recebido, sizeof(pid_recebido));
        if (bytes_recebidos != sizeof(pid_recebido)) {
            perror("Falha ao receber PID do filho");
            close(novo_socket);
            continue;
        }
        pids_filhos.push_back(pid_recebido);
    }

    // Sorteio do PID
    srand(time(NULL));
    int indice_sorteado = rand() % pids_filhos.size();
    pid_t pid_sorteado = pids_filhos[indice_sorteado];
    std::cout << "PID sorteado: " << pid_sorteado << std::endl;

    // Envia PID sorteado aos filhos
    for (int sock_cliente : sockets_clientes) {
        if (send(sock_cliente, &pid_sorteado, sizeof(pid_sorteado), 0) != sizeof(pid_sorteado)) {
            perror("Falha ao enviar PID sorteado ao filho");
        }
        close(sock_cliente);
    }

    close(server_fd);

    // Aguarda todos os processos filhos terminarem
    for (int i = 0; i < NUM_FILHOS; ++i) {
        wait(NULL);
    }

    return 0;
}
