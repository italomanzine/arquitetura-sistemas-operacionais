/**
 * Implemente aqui as funções do sistema de arquivos que simula EXT3
 */

#include "fs.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <algorithm>
#include <sstream>
#include <iomanip>

// Definição do Superbloco
struct SuperBlock {
    int blockSize;   // Tamanho de cada bloco em bytes
    int numBlocks;   // Número total de blocos
    int numInodes;   // Número total de inodes
};

// Função auxiliar para calcular deslocamentos
void calculateOffsets(const SuperBlock &sb, int &offsetBitmapBlocks, int &offsetBitmapInodes,
                      int &offsetInodes, int &offsetDataBlocks) {
    int superBlockSize = sizeof(SuperBlock);
    int bitmapBlocksSize = (sb.numBlocks + 7) / 8; // em bytes
    int bitmapInodesSize = (sb.numInodes + 7) / 8; // em bytes
    int inodesSize = sb.numInodes * sizeof(INODE);

    offsetBitmapBlocks = superBlockSize;
    offsetBitmapInodes = offsetBitmapBlocks + bitmapBlocksSize;
    offsetInodes = offsetBitmapInodes + bitmapInodesSize;
    offsetDataBlocks = offsetInodes + inodesSize;
}

// Função auxiliar para ler o superbloco
SuperBlock readSuperBlock(int fd) {
    SuperBlock sb;
    lseek(fd, 0, SEEK_SET);
    ssize_t bytesRead = read(fd, &sb, sizeof(SuperBlock));
    if (bytesRead != sizeof(SuperBlock)) {
        std::cerr << "Erro ao ler o superbloco.\n";
        exit(1);
    }
    return sb;
}

// Função auxiliar para ler um bitmap
void readBitmap(int fd, int offset, int size, std::vector<unsigned char> &bitmap) {
    bitmap.resize(size, 0x00);
    lseek(fd, offset, SEEK_SET);
    std::cerr << "Lendo bitmap no offset: " << offset << " com tamanho: " << size << " bytes.\n";
    ssize_t bytesRead = read(fd, bitmap.data(), size);
    if (bytesRead != size) {
        std::cerr << "Erro ao ler o bitmap. Bytes lidos: " << bytesRead << " esperados: " << size << ".\n";
        exit(1);
    }
}

// Função auxiliar para escrever um bitmap
void writeBitmap(int fd, int offset, const std::vector<unsigned char> &bitmap) {
    lseek(fd, offset, SEEK_SET);
    std::cerr << "Escrevendo bitmap no offset: " << offset << " com tamanho: " << bitmap.size() << " bytes.\n";
    ssize_t bytesWritten = write(fd, bitmap.data(), bitmap.size());
    if (bytesWritten != (ssize_t)bitmap.size()) {
        std::cerr << "Erro ao escrever o bitmap. Bytes escritos: " << bytesWritten << " esperados: " << bitmap.size() << ".\n";
        exit(1);
    }
}

// Função auxiliar para encontrar um inode livre
int findFreeInode(const std::vector<unsigned char> &bitmapInodes) {
    for (size_t i = 0; i < bitmapInodes.size(); ++i) {
        for (int bit = 0; bit < 8; ++bit) {
            if (!(bitmapInodes[i] & (1 << bit))) {
                return i * 8 + bit;
            }
        }
    }
    return -1; // Nenhum inode livre
}

// Função auxiliar para marcar um inode como usado ou livre
void setInodeBitmap(std::vector<unsigned char> &bitmapInodes, int inodeIndex, bool used) {
    int byteIndex = inodeIndex / 8;
    int bitIndex = inodeIndex % 8;
    if (byteIndex >= (int)bitmapInodes.size()) {
        std::cerr << "Erro: inodeIndex fora do bitmapInodes.\n";
        exit(1);
    }
    if (used) {
        bitmapInodes[byteIndex] |= (1 << bitIndex);
    } else {
        bitmapInodes[byteIndex] &= ~(1 << bitIndex);
    }
}

// Função auxiliar para ler um inode
INODE readInode(int fd, int offsetInodes, int inodeIndex) {
    INODE inode;
    lseek(fd, offsetInodes + inodeIndex * sizeof(INODE), SEEK_SET);
    ssize_t bytesRead = read(fd, &inode, sizeof(INODE));
    if (bytesRead != sizeof(INODE)) {
        std::cerr << "Erro ao ler o inode.\n";
        exit(1);
    }
    return inode;
}

// Função auxiliar para escrever um inode
void writeInode(int fd, int offsetInodes, int inodeIndex, const INODE &inode) {
    lseek(fd, offsetInodes + inodeIndex * sizeof(INODE), SEEK_SET);
    ssize_t bytesWritten = write(fd, &inode, sizeof(INODE));
    if (bytesWritten != sizeof(INODE)) {
        std::cerr << "Erro ao escrever o inode.\n";
        exit(1);
    }
}

// Função auxiliar para encontrar blocos livres
std::vector<int> findFreeBlocks(const std::vector<unsigned char> &bitmapBlocks, int numBlocksNeeded) {
    std::vector<int> freeBlocks;
    for (size_t i = 0; i < bitmapBlocks.size(); ++i) {
        for (int bit = 0; bit < 8; ++bit) {
            if (!(bitmapBlocks[i] & (1 << bit))) {
                int blockIndex = i * 8 + bit;
                if(blockIndex >= 0 && blockIndex < (int)(bitmapBlocks.size() * 8)) {
                    freeBlocks.push_back(blockIndex);
                    if (freeBlocks.size() == (size_t)numBlocksNeeded) {
                        return freeBlocks;
                    }
                }
            }
        }
    }
    return {}; // Não há blocos suficientes
}

// Função auxiliar para marcar um bloco como usado ou livre
void setBlockBitmap(std::vector<unsigned char> &bitmapBlocks, int blockIndex, bool used) {
    int byteIndex = blockIndex / 8;
    int bitIndex = blockIndex % 8;
    if (byteIndex >= (int)bitmapBlocks.size()) {
        std::cerr << "Erro: blockIndex fora do bitmapBlocks.\n";
        exit(1);
    }
    if (used) {
        bitmapBlocks[byteIndex] |= (1 << bitIndex);
    } else {
        bitmapBlocks[byteIndex] &= ~(1 << bitIndex);
    }
}

// Função auxiliar para atualizar o diretório pai
void updateParentDirectory(int fd, const SuperBlock &sb, int offsetInodes, int offsetDataBlocks, int parentInodeIndex, int childInodeIndex) {
    INODE parentInode = readInode(fd, offsetInodes, parentInodeIndex);

    // Ler o conteúdo do diretório pai
    std::vector<int> entries;
    if (parentInode.SIZE > 0) {
        int blockIndex = parentInode.DIRECT_BLOCKS[0];
        if(blockIndex >= sb.numBlocks){
            std::cerr << "Erro: Block index fora do range.\n";
            exit(1);
        }
        int dataOffset = offsetDataBlocks + blockIndex * sb.blockSize;
        lseek(fd, dataOffset, SEEK_SET);
        int numEntries = parentInode.SIZE / sizeof(int);
        entries.resize(numEntries);
        ssize_t bytesRead = read(fd, entries.data(), parentInode.SIZE);
        if (bytesRead != parentInode.SIZE) {
            std::cerr << "Erro ao ler o conteúdo do diretório pai.\n";
            exit(1);
        }
    }

    // Adicionar o novo inode ao diretório
    entries.push_back(childInodeIndex);
    parentInode.SIZE = entries.size() * sizeof(int);

    // Alocar bloco se necessário
    if (parentInode.SIZE > sb.blockSize) {
        std::cerr << "Erro: Diretório muito grande.\n";
        exit(1);
    }
    if (parentInode.DIRECT_BLOCKS[0] == 0) {
        // Encontrar um bloco livre
        std::vector<unsigned char> bitmapBlocks;
        readBitmap(fd, sizeof(SuperBlock), (sb.numBlocks + 7) / 8, bitmapBlocks);

        std::vector<int> freeBlocks = findFreeBlocks(bitmapBlocks, 1);
        if (freeBlocks.empty()) {
            std::cerr << "Erro: Sem blocos livres.\n";
            exit(1);
        }
        parentInode.DIRECT_BLOCKS[0] = freeBlocks[0];
        setBlockBitmap(bitmapBlocks, freeBlocks[0], true);
        writeBitmap(fd, sizeof(SuperBlock), bitmapBlocks);
    }

    // Escrever o conteúdo atualizado do diretório
    int blockIndex = parentInode.DIRECT_BLOCKS[0];
    int dataOffset = offsetDataBlocks + blockIndex * sb.blockSize;
    lseek(fd, dataOffset, SEEK_SET);
    ssize_t bytesWritten = write(fd, entries.data(), parentInode.SIZE);
    if (bytesWritten != parentInode.SIZE) {
        std::cerr << "Erro ao escrever o conteúdo atualizado do diretório.\n";
        exit(1);
    }

    // Atualizar o inode do diretório pai
    writeInode(fd, offsetInodes, parentInodeIndex, parentInode);
}

// Função auxiliar para navegar pelo caminho e encontrar o inode correspondente
int traversePath(int fd, const SuperBlock &sb, int offsetInodes, int offsetDataBlocks, const std::vector<std::string> &pathParts) {
    int currentInodeIndex = 0; // Supondo que o inode 0 é o root
    for (size_t i = 1; i < pathParts.size(); ++i) {
        INODE currentInode = readInode(fd, offsetInodes, currentInodeIndex);
        if (currentInode.IS_DIR != 0x01) {
            std::cerr << "Erro: Caminho inválido.\n";
            exit(1);
        }

        // Ler as entradas do diretório
        std::vector<int> entries;
        if (currentInode.SIZE > 0) {
            int blockIndex = currentInode.DIRECT_BLOCKS[0];
            if(blockIndex >= sb.numBlocks){
                std::cerr << "Erro: Block index fora do range.\n";
                exit(1);
            }
            int dataOffset = offsetDataBlocks + blockIndex * sb.blockSize;
            lseek(fd, dataOffset, SEEK_SET);
            int numEntries = currentInode.SIZE / sizeof(int);
            entries.resize(numEntries);
            ssize_t bytesRead = read(fd, entries.data(), currentInode.SIZE);
            if (bytesRead != currentInode.SIZE) {
                std::cerr << "Erro ao ler as entradas do diretório.\n";
                exit(1);
            }
        }

        bool found = false;
        for (int inodeIndex : entries) {
            INODE inode = readInode(fd, offsetInodes, inodeIndex);
            if (inode.IS_USED == 0x01 && std::string(inode.NAME) == pathParts[i]) {
                currentInodeIndex = inodeIndex;
                found = true;
                break;
            }
        }
        if (!found) {
            return -1; // Caminho não encontrado
        }
    }
    return currentInodeIndex;
}

// Implementação da função initFs
void initFs(std::string fsFileName, int blockSize, int numBlocks, int numInodes) {
    // Abrir ou criar o arquivo em disco
    int fd = open(fsFileName.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd < 0) {
        std::cerr << "Erro ao criar o arquivo do sistema de arquivos.\n";
        exit(1);
    }

    // Preparar SuperBlock
    SuperBlock sb;
    sb.blockSize = blockSize;
    sb.numBlocks = numBlocks;
    sb.numInodes = numInodes;

    // Inicializar bitmaps
    int bitmapBlocksSize = (sb.numBlocks + 7) / 8;
    int bitmapInodesSize = (sb.numInodes + 7) / 8;
    std::vector<unsigned char> bitmapBlocks(bitmapBlocksSize, 0x00);
    std::vector<unsigned char> bitmapInodes(bitmapInodesSize, 0x00);

    // Marcar inode0 como usado (root)
    setInodeBitmap(bitmapInodes, 0, true);

    // Inicializar inodes (root e vazios)
    INODE rootInode;
    memset(&rootInode, 0, sizeof(INODE));
    rootInode.IS_USED = 0x01;
    rootInode.IS_DIR = 0x01;
    strncpy(rootInode.NAME, "/", 10);
    rootInode.SIZE = 0;

    // Alocar um bloco para o diretório raiz
    std::vector<int> freeBlocks = findFreeBlocks(bitmapBlocks, 1);
    if (freeBlocks.empty()) {
        std::cerr << "Erro: Não há blocos livres para o diretório raiz.\n";
        close(fd);
        exit(1);
    }
    int rootDataBlock = freeBlocks[0];
    setBlockBitmap(bitmapBlocks, rootDataBlock, true);
    rootInode.DIRECT_BLOCKS[0] = rootDataBlock;
    rootInode.SIZE = 2 * sizeof(int); // Tamanho para '.' e '..'

    // Escrever o SuperBlock
    lseek(fd, 0, SEEK_SET);
    ssize_t bytesWritten = write(fd, &sb, sizeof(SuperBlock));
    if (bytesWritten != sizeof(SuperBlock)) {
        std::cerr << "Erro ao escrever o SuperBlock.\n";
        close(fd);
        exit(1);
    }

    // Escrever bitmapBlocks
    writeBitmap(fd, sizeof(SuperBlock), bitmapBlocks);

    // Escrever bitmapInodes
    writeBitmap(fd, sizeof(SuperBlock) + bitmapBlocksSize, bitmapInodes);

    // Escrever inodes
    writeInode(fd, sizeof(SuperBlock) + bitmapBlocksSize + bitmapInodesSize, 0, rootInode);

    // Escrever inodes vazios
    INODE emptyInode;
    memset(&emptyInode, 0, sizeof(INODE));
    for(int i=1; i<numInodes; i++) {
        writeInode(fd, sizeof(SuperBlock) + bitmapBlocksSize + bitmapInodesSize, i, emptyInode);
    }

    // Escrever o bloco de dados para o diretório raiz com entradas '.' e '..'
    std::vector<int> rootEntries = {0, 0}; // '.' e '..' apontando para o inode 0
    if (rootEntries.size() * sizeof(int) > sb.blockSize) {
        std::cerr << "Erro: Bloco de dados do diretório raiz muito pequeno para as entradas.\n";
        close(fd);
        exit(1);
    }
    int dataOffset = sizeof(SuperBlock) + bitmapBlocksSize + bitmapInodesSize + numInodes * sizeof(INODE) + rootDataBlock * sb.blockSize;
    lseek(fd, dataOffset, SEEK_SET);
    bytesWritten = write(fd, rootEntries.data(), rootEntries.size() * sizeof(int));
    if (bytesWritten != (ssize_t)(rootEntries.size() * sizeof(int))) {
        std::cerr << "Erro ao escrever as entradas do diretório raiz.\n";
        close(fd);
        exit(1);
    }

    // Escrever o bloco de dados do diretório raiz
    std::vector<char> directoryData(sb.blockSize, 0x00);
    // Opcional: preencha o restante do bloco com zeros ou outras informações, se necessário
    // Neste caso, já escrevemos as entradas, então podemos deixar o restante como zeros

    // Atualizar o inode raiz com o tamanho correto
    writeInode(fd, sizeof(SuperBlock) + bitmapBlocksSize + bitmapInodesSize, 0, rootInode);

    // Preencher o restante do arquivo com zeros
    int totalSize = sizeof(SuperBlock) + bitmapBlocksSize + bitmapInodesSize + numInodes * sizeof(INODE) + numBlocks * sb.blockSize;
    lseek(fd, totalSize - 1, SEEK_SET);
    write(fd, "", 1);

    close(fd);
}

// Implementação da função addFile
void addFile(std::string fsFileName, std::string filePath, std::string fileContent) {
    int fd = open(fsFileName.c_str(), O_RDWR);
    if (fd < 0) {
        std::cerr << "Erro ao abrir o arquivo do sistema de arquivos.\n";
        exit(1);
    }

    SuperBlock sb = readSuperBlock(fd);

    int offsetBitmapBlocks, offsetBitmapInodes, offsetInodes, offsetDataBlocks;
    calculateOffsets(sb, offsetBitmapBlocks, offsetBitmapInodes, offsetInodes, offsetDataBlocks);

    // Ler bitmaps
    std::vector<unsigned char> bitmapBlocks;
    readBitmap(fd, offsetBitmapBlocks, (sb.numBlocks + 7) / 8, bitmapBlocks);

    std::vector<unsigned char> bitmapInodes;
    readBitmap(fd, offsetBitmapInodes, (sb.numInodes + 7) / 8, bitmapInodes);

    // Encontrar inode livre
    int freeInodeIndex = findFreeInode(bitmapInodes);
    if (freeInodeIndex == -1) {
        std::cerr << "Erro: Sem inodes livres.\n";
        close(fd);
        exit(1);
    }

    // Marcar inode como usado
    setInodeBitmap(bitmapInodes, freeInodeIndex, true);
    writeBitmap(fd, offsetBitmapInodes, bitmapInodes);

    // Atualizar inode
    INODE newInode;
    memset(&newInode, 0, sizeof(INODE));
    newInode.IS_USED = 0x01;
    newInode.IS_DIR = 0x00;
    std::string fileName = filePath.substr(filePath.find_last_of('/') + 1);
    strncpy(newInode.NAME, fileName.c_str(), 10);
    newInode.SIZE = fileContent.size();

    // Alocar blocos de dados
    int numBlocksNeeded = (fileContent.size() + sb.blockSize - 1) / sb.blockSize;
    if (numBlocksNeeded > 3) {
        std::cerr << "Erro: Arquivo muito grande.\n";
        close(fd);
        exit(1);
    }

    std::vector<int> freeBlocksForFile = findFreeBlocks(bitmapBlocks, numBlocksNeeded);
    if (freeBlocksForFile.size() < (size_t)numBlocksNeeded) {
        std::cerr << "Erro: Sem blocos livres suficientes.\n";
        close(fd);
        exit(1);
    }

    // Marcar blocos como usados e atribuir ao inode
    for (int i = 0; i < numBlocksNeeded; ++i) {
        setBlockBitmap(bitmapBlocks, freeBlocksForFile[i], true);
        newInode.DIRECT_BLOCKS[i] = freeBlocksForFile[i];
    }
    writeBitmap(fd, offsetBitmapBlocks, bitmapBlocks);

    // Escrever o conteúdo nos blocos de dados
    for (int i = 0; i < numBlocksNeeded; ++i) {
        int blockIndex = freeBlocksForFile[i];
        int dataOffsetBlock = offsetDataBlocks + blockIndex * sb.blockSize;
        lseek(fd, dataOffsetBlock, SEEK_SET);
        int bytesToWrite = std::min((int)fileContent.size() - i * sb.blockSize, sb.blockSize);
        write(fd, fileContent.data() + i * sb.blockSize, bytesToWrite);
    }

    // Escrever o inode
    writeInode(fd, offsetInodes, freeInodeIndex, newInode);

    // Atualizar o diretório pai
    // Parse filePath into directory components
    std::vector<std::string> pathParts;
    std::stringstream ss(filePath);
    std::string token;
    while (std::getline(ss, token, '/')) {
        if (!token.empty()) {
            pathParts.push_back(token);
        }
    }
    // Insert root at beginning
    pathParts.insert(pathParts.begin(), "/");

    if (pathParts.size() <= 1) {
        // Diretório raiz
        updateParentDirectory(fd, sb, offsetInodes, offsetDataBlocks, 0, freeInodeIndex);
    } else {
        // Encontrar o diretório pai
        pathParts.pop_back(); // Remover o nome do arquivo
        int parentInodeIndex = traversePath(fd, sb, offsetInodes, offsetDataBlocks, pathParts);
        if (parentInodeIndex == -1) {
            std::cerr << "Erro: Diretório pai não encontrado.\n";
            close(fd);
            exit(1);
        }
        updateParentDirectory(fd, sb, offsetInodes, offsetDataBlocks, parentInodeIndex, freeInodeIndex);
    }

    close(fd);
}

// Implementação da função addDir
void addDir(std::string fsFileName, std::string dirPath) {
    int fd = open(fsFileName.c_str(), O_RDWR);
    if (fd < 0) {
        std::cerr << "Erro ao abrir o arquivo do sistema de arquivos.\n";
        exit(1);
    }

    SuperBlock sb = readSuperBlock(fd);

    int offsetBitmapBlocks, offsetBitmapInodes, offsetInodes, offsetDataBlocks;
    calculateOffsets(sb, offsetBitmapBlocks, offsetBitmapInodes, offsetInodes, offsetDataBlocks);

    // Ler bitmaps
    std::vector<unsigned char> bitmapBlocks;
    readBitmap(fd, offsetBitmapBlocks, (sb.numBlocks + 7) / 8, bitmapBlocks);

    std::vector<unsigned char> bitmapInodes;
    readBitmap(fd, offsetBitmapInodes, (sb.numInodes + 7) / 8, bitmapInodes);

    // Encontrar inode livre
    int freeInodeIndex = findFreeInode(bitmapInodes);
    if (freeInodeIndex == -1) {
        std::cerr << "Erro: Sem inodes livres.\n";
        close(fd);
        exit(1);
    }

    // Marcar inode como usado
    setInodeBitmap(bitmapInodes, freeInodeIndex, true);
    writeBitmap(fd, offsetBitmapInodes, bitmapInodes);

    // Atualizar inode
    INODE newInode;
    memset(&newInode, 0, sizeof(INODE));
    newInode.IS_USED = 0x01;
    newInode.IS_DIR = 0x01;
    std::string dirName = dirPath.substr(dirPath.find_last_of('/') + 1);
    strncpy(newInode.NAME, dirName.c_str(), 10);
    newInode.SIZE = 0;

    // Alocar um bloco para o diretório
    std::vector<int> freeBlocksForDir = findFreeBlocks(bitmapBlocks, 1);
    if (freeBlocksForDir.empty()) {
        std::cerr << "Erro: Não há blocos livres para o diretório.\n";
        close(fd);
        exit(1);
    }
    int dirDataBlock = freeBlocksForDir[0];
    setBlockBitmap(bitmapBlocks, dirDataBlock, true);
    newInode.DIRECT_BLOCKS[0] = dirDataBlock;
    newInode.SIZE = 2 * sizeof(int); // Tamanho para '.' e '..'

    // Escrever o inode
    writeInode(fd, offsetInodes, freeInodeIndex, newInode);

    // Escrever as entradas '.' e '..' no novo diretório
    std::vector<int> dirEntries = {freeInodeIndex, freeInodeIndex}; // '.' e '..' apontando para si mesmo
    if (dirEntries.size() * sizeof(int) > sb.blockSize) {
        std::cerr << "Erro: Bloco de dados do novo diretório muito pequeno para as entradas.\n";
        close(fd);
        exit(1);
    }
    int dataOffsetDir = offsetDataBlocks + dirDataBlock * sb.blockSize;
    lseek(fd, dataOffsetDir, SEEK_SET);
    ssize_t bytesWritten = write(fd, dirEntries.data(), dirEntries.size() * sizeof(int));
    if (bytesWritten != (ssize_t)(dirEntries.size() * sizeof(int))) {
        std::cerr << "Erro ao escrever as entradas do novo diretório.\n";
        close(fd);
        exit(1);
    }

    // Atualizar o inode do novo diretório com o tamanho correto
    writeInode(fd, offsetInodes, freeInodeIndex, newInode);

    // Escrever o bloco de dados do novo diretório
    std::vector<char> directoryData(sb.blockSize, 0x00);
    // Opcional: preencha o restante do bloco com zeros ou outras informações, se necessário

    // Escrever o bitmapBlocks atualizado
    writeBitmap(fd, offsetBitmapBlocks, bitmapBlocks);

    // Atualizar o diretório pai
    // Parse dirPath into directory components
    std::vector<std::string> pathParts;
    std::stringstream ss(dirPath);
    std::string token;
    while (std::getline(ss, token, '/')) {
        if (!token.empty()) {
            pathParts.push_back(token);
        }
    }
    // Insert root at beginning
    pathParts.insert(pathParts.begin(), "/");

    if (pathParts.size() <= 1) {
        // Diretório raiz
        updateParentDirectory(fd, sb, offsetInodes, offsetDataBlocks, 0, freeInodeIndex);
    } else {
        // Encontrar o diretório pai
        pathParts.pop_back(); // Remover o nome do diretório
        int parentInodeIndex = traversePath(fd, sb, offsetInodes, offsetDataBlocks, pathParts);
        if (parentInodeIndex == -1) {
            std::cerr << "Erro: Diretório pai não encontrado.\n";
            close(fd);
            exit(1);
        }
        updateParentDirectory(fd, sb, offsetInodes, offsetDataBlocks, parentInodeIndex, freeInodeIndex);
    }

    close(fd);
}

// Função auxiliar para remover um inode (recursivamente)
void removeInode(int fd, const SuperBlock &sb, int offsetBitmapBlocks, int offsetBitmapInodes, int offsetInodes, int offsetDataBlocks, int inodeIndex) {
    INODE inode = readInode(fd, offsetInodes, inodeIndex);

    if (inode.IS_DIR == 0x01) {
        // Diretório: remover recursivamente
        std::vector<int> entries;
        if (inode.SIZE > 0) {
            int blockIndex = inode.DIRECT_BLOCKS[0];
            if(blockIndex >= sb.numBlocks){
                std::cerr << "Erro: Block index fora do range.\n";
                exit(1);
            }
            int dataOffsetDir = offsetDataBlocks + blockIndex * sb.blockSize;
            lseek(fd, dataOffsetDir, SEEK_SET);
            int numEntries = inode.SIZE / sizeof(int);
            entries.resize(numEntries);
            ssize_t bytesRead = read(fd, entries.data(), inode.SIZE);
            if (bytesRead != inode.SIZE) {
                std::cerr << "Erro ao ler as entradas do diretório.\n";
                exit(1);
            }
        }

        for (int childInodeIndex : entries) {
            removeInode(fd, sb, offsetBitmapBlocks, offsetBitmapInodes, offsetInodes, offsetDataBlocks, childInodeIndex);
        }

        // Liberar bloco de dados do diretório
        if (inode.DIRECT_BLOCKS[0] != 0) {
            std::vector<unsigned char> bitmapBlocks;
            readBitmap(fd, offsetBitmapBlocks, (sb.numBlocks + 7) / 8, bitmapBlocks);
            setBlockBitmap(bitmapBlocks, inode.DIRECT_BLOCKS[0], false);
            writeBitmap(fd, offsetBitmapBlocks, bitmapBlocks);
        }
    } else {
        // Arquivo: liberar blocos de dados
        std::vector<unsigned char> bitmapBlocks;
        readBitmap(fd, offsetBitmapBlocks, (sb.numBlocks + 7) / 8, bitmapBlocks);
        for (int i = 0; i < 3; ++i) {
            if (inode.DIRECT_BLOCKS[i] != 0) {
                setBlockBitmap(bitmapBlocks, inode.DIRECT_BLOCKS[i], false);
            }
        }
        writeBitmap(fd, offsetBitmapBlocks, bitmapBlocks);
    }

    // Marcar inode como livre
    std::vector<unsigned char> bitmapInodes;
    readBitmap(fd, offsetBitmapInodes, (sb.numInodes + 7) / 8, bitmapInodes);
    setInodeBitmap(bitmapInodes, inodeIndex, false);
    writeBitmap(fd, offsetBitmapInodes, bitmapInodes);

    // Limpar o inode
    memset(&inode, 0, sizeof(INODE));
    writeInode(fd, offsetInodes, inodeIndex, inode);
}

// Implementação da função remove
void remove(std::string fsFileName, std::string path) {
    int fd = open(fsFileName.c_str(), O_RDWR);
    if (fd < 0) {
        std::cerr << "Erro ao abrir o arquivo do sistema de arquivos.\n";
        exit(1);
    }

    SuperBlock sb = readSuperBlock(fd);

    int offsetBitmapBlocks, offsetBitmapInodes, offsetInodes, offsetDataBlocks;
    calculateOffsets(sb, offsetBitmapBlocks, offsetBitmapInodes, offsetInodes, offsetDataBlocks);

    // Encontrar o inode
    std::vector<std::string> pathParts;
    std::stringstream ss(path);
    std::string token;
    while (std::getline(ss, token, '/')) {
        if (!token.empty()) {
            pathParts.push_back(token);
        }
    }
    // Insert root at beginning
    pathParts.insert(pathParts.begin(), "/");

    int inodeIndex = traversePath(fd, sb, offsetInodes, offsetDataBlocks, pathParts);
    if (inodeIndex == -1) {
        std::cerr << "Erro: Caminho não encontrado.\n";
        close(fd);
        exit(1);
    }

    // Remover o inode
    removeInode(fd, sb, offsetBitmapBlocks, offsetBitmapInodes, offsetInodes, offsetDataBlocks, inodeIndex);

    // Atualizar o diretório pai
    pathParts.pop_back(); // Remover o último elemento
    int parentInodeIndex = traversePath(fd, sb, offsetInodes, offsetDataBlocks, pathParts);
    if (parentInodeIndex != -1) {
        INODE parentInode = readInode(fd, offsetInodes, parentInodeIndex);

        // Ler as entradas do diretório pai
        std::vector<int> entries;
        if (parentInode.SIZE > 0) {
            int blockIndex = parentInode.DIRECT_BLOCKS[0];
            if(blockIndex >= sb.numBlocks){
                std::cerr << "Erro: Block index fora do range.\n";
                exit(1);
            }
            int dataOffsetParent = offsetDataBlocks + blockIndex * sb.blockSize;
            lseek(fd, dataOffsetParent, SEEK_SET);
            int numEntries = parentInode.SIZE / sizeof(int);
            entries.resize(numEntries);
            ssize_t bytesRead = read(fd, entries.data(), parentInode.SIZE);
            if (bytesRead != parentInode.SIZE) {
                std::cerr << "Erro ao ler as entradas do diretório pai.\n";
                exit(1);
            }
        }

        // Remover o inode das entradas
        entries.erase(std::remove(entries.begin(), entries.end(), inodeIndex), entries.end());
        parentInode.SIZE = entries.size() * sizeof(int);

        // Escrever o conteúdo atualizado do diretório pai
        int blockIndex = parentInode.DIRECT_BLOCKS[0];
        int dataOffsetParent = offsetDataBlocks + blockIndex * sb.blockSize;
        lseek(fd, dataOffsetParent, SEEK_SET);
        ssize_t bytesWritten = write(fd, entries.data(), parentInode.SIZE);
        if (bytesWritten != parentInode.SIZE) {
            std::cerr << "Erro ao escrever o conteúdo atualizado do diretório pai.\n";
            exit(1);
        }

        // Atualizar o inode do diretório pai
        writeInode(fd, offsetInodes, parentInodeIndex, parentInode);
    }

    close(fd);
}

// Implementação da função move
void move(std::string fsFileName, std::string oldPath, std::string newPath) {
    int fd = open(fsFileName.c_str(), O_RDWR);
    if (fd < 0) {
        std::cerr << "Erro ao abrir o arquivo do sistema de arquivos.\n";
        exit(1);
    }

    SuperBlock sb = readSuperBlock(fd);

    int offsetBitmapBlocks, offsetBitmapInodes, offsetInodes, offsetDataBlocks;
    calculateOffsets(sb, offsetBitmapBlocks, offsetBitmapInodes, offsetInodes, offsetDataBlocks);

    // Encontrar o inode a ser movido
    std::vector<std::string> oldPathParts;
    std::stringstream ssOld(oldPath);
    std::string tokenOld;
    while (std::getline(ssOld, tokenOld, '/')) {
        if (!tokenOld.empty()) {
            oldPathParts.push_back(tokenOld);
        }
    }
    // Insert root at beginning
    oldPathParts.insert(oldPathParts.begin(), "/");

    int inodeIndex = traversePath(fd, sb, offsetInodes, offsetDataBlocks, oldPathParts);
    if (inodeIndex == -1) {
        std::cerr << "Erro: Caminho antigo não encontrado.\n";
        close(fd);
        exit(1);
    }

    // Atualizar o nome no inode
    INODE inode = readInode(fd, offsetInodes, inodeIndex);
    std::string newName = newPath.substr(newPath.find_last_of('/') + 1);
    strncpy(inode.NAME, newName.c_str(), 10);
    writeInode(fd, offsetInodes, inodeIndex, inode);

    // Remover do diretório pai antigo
    oldPathParts.pop_back(); // Remover o último elemento
    int oldParentInodeIndex = traversePath(fd, sb, offsetInodes, offsetDataBlocks, oldPathParts);
    if (oldParentInodeIndex != -1) {
        INODE oldParentInode = readInode(fd, offsetInodes, oldParentInodeIndex);

        // Ler as entradas do diretório pai antigo
        std::vector<int> entries;
        if (oldParentInode.SIZE > 0) {
            int blockIndex = oldParentInode.DIRECT_BLOCKS[0];
            if(blockIndex >= sb.numBlocks){
                std::cerr << "Erro: Block index fora do range.\n";
                exit(1);
            }
            int dataOffsetOldParent = offsetDataBlocks + blockIndex * sb.blockSize;
            lseek(fd, dataOffsetOldParent, SEEK_SET);
            int numEntries = oldParentInode.SIZE / sizeof(int);
            entries.resize(numEntries);
            ssize_t bytesRead = read(fd, entries.data(), oldParentInode.SIZE);
            if (bytesRead != oldParentInode.SIZE) {
                std::cerr << "Erro ao ler as entradas do diretório pai antigo.\n";
                exit(1);
            }
        }

        // Remover o inode das entradas
        entries.erase(std::remove(entries.begin(), entries.end(), inodeIndex), entries.end());
        oldParentInode.SIZE = entries.size() * sizeof(int);

        // Escrever o conteúdo atualizado do diretório pai antigo
        int blockIndex = oldParentInode.DIRECT_BLOCKS[0];
        int dataOffsetOldParent = offsetDataBlocks + blockIndex * sb.blockSize;
        lseek(fd, dataOffsetOldParent, SEEK_SET);
        ssize_t bytesWritten = write(fd, entries.data(), oldParentInode.SIZE);
        if (bytesWritten != oldParentInode.SIZE) {
            std::cerr << "Erro ao escrever o conteúdo atualizado do diretório pai antigo.\n";
            exit(1);
        }

        // Atualizar o inode do diretório pai antigo
        writeInode(fd, offsetInodes, oldParentInodeIndex, oldParentInode);
    }

    // Adicionar ao novo diretório pai
    std::vector<std::string> newPathParts;
    std::stringstream ssNew(newPath);
    std::string tokenNew;
    while (std::getline(ssNew, tokenNew, '/')) {
        if (!tokenNew.empty()) {
            newPathParts.push_back(tokenNew);
        }
    }
    // Insert root at beginning
    newPathParts.insert(newPathParts.begin(), "/");

    if (newPathParts.size() <= 1) {
        // Diretório raiz
        updateParentDirectory(fd, sb, offsetInodes, offsetDataBlocks, 0, inodeIndex);
    } else {
        // Encontrar o diretório pai
        newPathParts.pop_back(); // Remover o nome do arquivo/diretório
        int newParentInodeIndex = traversePath(fd, sb, offsetInodes, offsetDataBlocks, newPathParts);
        if (newParentInodeIndex == -1) {
            std::cerr << "Erro: Novo diretório pai não encontrado.\n";
            close(fd);
            exit(1);
        }
        updateParentDirectory(fd, sb, offsetInodes, offsetDataBlocks, newParentInodeIndex, inodeIndex);
    }

    close(fd);
}
