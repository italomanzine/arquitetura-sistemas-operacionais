// sha256.cpp
#include "sha256.h"
#include <iostream>
#include <sstream>
#include <openssl/evp.h>

std::string printSha256(const char *path)
{
    // Inicializar o contexto
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL) {
        std::cerr << "Erro ao criar o contexto EVP_MD_CTX." << std::endl;
        exit(1);
    }

    const EVP_MD *md = EVP_sha256();

    if (EVP_DigestInit_ex(mdctx, md, NULL) != 1) {
        std::cerr << "Erro ao inicializar o contexto EVP_DigestInit_ex." << std::endl;
        EVP_MD_CTX_free(mdctx);
        exit(1);
    }

    // Ler o arquivo e atualizar o contexto
    FILE *file = fopen(path, "rb");
    if (!file) {
        std::cerr << "Erro ao abrir o arquivo para calcular o SHA-256." << std::endl;
        EVP_MD_CTX_free(mdctx);
        exit(1);
    }

    unsigned char buffer[1024];
    size_t bytesRead = 0;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (EVP_DigestUpdate(mdctx, buffer, bytesRead) != 1) {
            std::cerr << "Erro ao atualizar o hash com EVP_DigestUpdate." << std::endl;
            fclose(file);
            EVP_MD_CTX_free(mdctx);
            exit(1);
        }
    }
    fclose(file);

    // Finalizar o contexto e obter o hash
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;

    if (EVP_DigestFinal_ex(mdctx, md_value, &md_len) != 1) {
        std::cerr << "Erro ao finalizar o hash com EVP_DigestFinal_ex." << std::endl;
        EVP_MD_CTX_free(mdctx);
        exit(1);
    }

    EVP_MD_CTX_free(mdctx);

    // Converter o hash para string hexadecimal
    std::stringstream ss;
    for (unsigned int i = 0; i < md_len; i++) {
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)md_value[i];
        if (i != md_len - 1) {
            ss << ":";
        }
    }

    return ss.str();
}
