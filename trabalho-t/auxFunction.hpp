#ifndef auxFunction_hpp
#define auxFunction_hpp

#include "fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <cstring>
#include <iostream>
#include <functional>

using namespace std;

// Tamanho do bloco: char = 1 byte
// Tabanho do numero de blocos: char = 1 byte
// Tamanho do numero de inodes: char = 1 byte
// Tamanho do mapa de bits: ceil(numBlocks/8.0)
// Tamanho do vetor de inodes: numInodes * sizeof(INODE)
// Tamanho do diretório raiz: char = 1 byte
// Tamanho do vetor de blocos: numBlocks * blockSize

// Tamanho do inode: 22 bytes
// Tamanho do IS_USED: char = 1 byte
// Tamanho do IS_DIR: char = 1 byte
// Tamanho do NAME: char[10] = 10 bytes
// Tamanho do SIZE: char = 1 byte
// Tamanho do DIRECT_BLOCKS: char[3] = 3 bytes
// Tamanho do INDIRECT_BLOCKS: char[3] = 3 bytes
// Tamanho do DOUBLE_INDIRECT_BLOCKS: char[3] = 3 bytes

// Função para retornar o tamanho do mapa de bits
// Dividir a quantidade de blocos por 8 e arredondar para cima o resultado.
int getBitMapSize(int numBlocks)
{
  return (int)ceil(numBlocks / 8.0);
}

// Função para pegar o primeiro inode livre
int getFreeInode(unsigned char numInodes, vector<INODE> inodes)
{
  int inodeIndex = 0;
  for (int i = 0; i < numInodes; i++)
  {
    if (inodes[i].IS_USED == 0x00)
    {
      inodeIndex = i;
      break;
    }
  }
  return inodeIndex;
}

// Função para fazer o mapeamento dos blocos usados
vector<bool> mappingUsedBlocks(vector<INODE> inodes, unsigned char numBlocks, unsigned char numInodes)
{
  vector<bool> usedBlocks(numBlocks, false);
  usedBlocks[0] = true;

  for (int i = 0; i < numInodes; i++)
  {
    if (inodes[i].IS_USED == 0x01)
    {
      for (int j = 0; j < 3; j++)
      {
        if (inodes[i].DIRECT_BLOCKS[j] != 0x00)
        {
          usedBlocks[inodes[i].DIRECT_BLOCKS[j]] = true;
        }
        if (inodes[i].INDIRECT_BLOCKS[j] != 0x00)
        {
          usedBlocks[inodes[i].INDIRECT_BLOCKS[j]] = true;
        }
        if (inodes[i].DOUBLE_INDIRECT_BLOCKS[j] != 0x00)
        {
          usedBlocks[inodes[i].DOUBLE_INDIRECT_BLOCKS[j]] = true;
        }
      }
    }
  }
  return usedBlocks;
}

// Função para obter o nome do pai do arquivo ou diretório a ser criado.
// O pai é o primeiro nome antes do ultimo "/.
// Ex: /home/usuario/arquivo.txt -> "usuario/"
string getFatherName(string Path)
{
  string fatherName = "";
  int ultimaBarra = Path.find_last_of("/");

  for (int i = 1; i < ultimaBarra; i++)
  {
    fatherName += Path[i];
  }
  if (fatherName == "")
  {
    fatherName = "/";
  }
  return fatherName;
}

// Função para obter o nome do arquivo ou diretório a ser removido
// Pega o último nome depois da última barra.
string getName(string Path)
{
  string name = "";
  int ultimaBarra = Path.find_last_of("/");
  for (int i = ultimaBarra + 1; i < Path.length(); i++)
  {
    name += Path[i];
  }
  return name;
}

// Função para obter o índice do inode pelo nome.
int getInodeIndex(string nome, vector<INODE> inodes, int numInodes)
{
  for (int i = 0; i < numInodes; i++)
  {
    string nomeAux = "";
    for (int j = 0; j < 10; j++)
    {
      if (inodes[i].NAME[j] != 0x00)
      {
        nomeAux += inodes[i].NAME[j];
      }
    }

    if (nomeAux == nome)
    {
      return i;
    }
  }
  return -1;
}

/**
 * @brief Faz a inicialização do arquivo EXT3 usando o arquivo aberto.
 * @param arquivo arquivo aberto que simula EXT3
 * @param blockSize tamanho em bytes do bloco
 * @param numBlocks quantidade de blocos
 * @param numInodes quantidade de inodes
 */
void inicializar(FILE *arquivo, int blockSize, int numBlocks, int numInodes)
{
  // Gravando os três primeiros bytes do arquivo.
  fwrite(&blockSize, 1, 1, arquivo);
  fwrite(&numBlocks, 1, 1, arquivo);
  fwrite(&numInodes, 1, 1, arquivo);

  // Quantidade de bytes que o Mapa de Bits irá ocupar.
  int bitMapSize = getBitMapSize(numBlocks);

  // Espaço para o Mapa de Bits.
  vector<unsigned char> bitMap(bitMapSize);

  // Gravar o mapa de bits no arquivo
  // Apenas o primeiro bloco está sendo usado, pois é o bloco do diretório raiz.
  bitMap[0] = 0x01;
  for (int i = 1; i < bitMapSize; i++)
  {
    bitMap[i] = 0x00;
  }
  fwrite(&bitMap[0], sizeof(unsigned char), bitMapSize, arquivo);

  // Espaço para o vetor de inodes.
  vector<INODE> inodes(numInodes);

  // Raiz do sistema de arquivos é sempre 0.
  unsigned char root = 0x00;

  // Primeiro inode = inode do diretório raiz.
  // O inode do diretório raiz recebe 0x01 em usado, 0x01 em tipo, 0x00 em tamanho, '/' em nome e 0x00 em todos os blocos.
  // O nome é um char de 10 espaços, então é necessário preencher com 0x00.
  inodes[root].IS_USED = 0x01;
  inodes[root].IS_DIR = 0x01;
  inodes[root].NAME[0] = '/';
  for (int i = 1; i < 10; i++)
  {
    inodes[root].NAME[i] = 0x00;
  }
  inodes[root].SIZE = 0x00;
  for (int i = 0; i < 3; i++)
  {
    inodes[root].DIRECT_BLOCKS[i] = 0x00;
    inodes[root].INDIRECT_BLOCKS[i] = 0x00;
    inodes[root].DOUBLE_INDIRECT_BLOCKS[i] = 0x00;
  }

  // Os demais inodes recebem 0x00 em tudo.
  for (int i = 1; i < numInodes; i++)
  {
    inodes[i].IS_USED = 0x00;
    inodes[i].IS_DIR = 0x00;
    inodes[i].SIZE = 0x00;
    for (int j = 0; j < 10; j++)
    {
      inodes[i].NAME[j] = 0x00;
    }
    for (int j = 0; j < 3; j++)
    {
      inodes[i].DIRECT_BLOCKS[j] = 0x00;
      inodes[i].INDIRECT_BLOCKS[j] = 0x00;
      inodes[i].DOUBLE_INDIRECT_BLOCKS[j] = 0x00;
    }
  }

  // Gravando o vetor de inodes no arquivo após o mapa de bits.
  fwrite(&inodes[0], sizeof(INODE), numInodes, arquivo);

  // Gravando o indice do inode do diretório raiz no arquivo após o vetor de inodes.
  fwrite(&root, sizeof(unsigned char), 1, arquivo);

  // Espaço para o vetor de blocos.
  vector<vector<unsigned char>> blocos(numBlocks, vector<unsigned char>(blockSize));

  // Na inicialização, todos os blocos recebem 0x00.
  for (int i = 0; i < numBlocks; i++)
  {
    for (int j = 0; j < blockSize; j++)
    {
      blocos[i][j] = 0x00;
    }
  }

  // Gravando o vetor de blocos no arquivo após o indice do inode do diretório raiz.
  for (int i = 0; i < numBlocks; i++)
  {
    fwrite(&blocos[i][0], sizeof(unsigned char), blockSize, arquivo);
  }
}

/**
 * @brief Adiciona um novo arquivo dentro do sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param arquivo arquivo aberto que contém um sistema de arquivos que simula EXT3.
 * @param filePath caminho completo novo arquivo dentro sistema de arquivos que simula EXT3.
 * @param fileContent conteúdo do novo arquivo
 */
void adicionarArquivo(FILE *arquivo, string filePath, string fileContent)
{
  // blockSize: tamanho do bloco
  // numBlocks: quantidade de blocos
  // numInodes: quantidade de inodes
  // root: inode do diretório raiz
  unsigned char blockSize, numBlocks, numInodes, root;

  // Posicionamento do ponteiro no inicio do arquivo e leitura dos 3 primeiros bytes.
  fseek(arquivo, 0, SEEK_SET);
  fread(&blockSize, sizeof(unsigned char), 1, arquivo);
  fread(&numBlocks, sizeof(unsigned char), 1, arquivo);
  fread(&numInodes, sizeof(unsigned char), 1, arquivo);

  // Quantidade de bytes do mapa de bits.
  unsigned char bitMapSize = getBitMapSize(numBlocks);

  // Espaço para tratamento do mapa de bits
  vector<unsigned char> bitMap(bitMapSize);

  // Espaço para tratamento dos inodes.
  vector<INODE> inodes(numInodes);

  // Espaço para o vetor de blocos.
  vector<vector<unsigned char>> blocos(numBlocks, vector<unsigned char>(blockSize));

  // Leitura dos bytes do mapa de bits, inodes, root e blocos.
  fread(&bitMap[0], sizeof(unsigned char), bitMapSize, arquivo);
  fread(&inodes[0], sizeof(INODE), numInodes, arquivo);
  fread(&root, 1, 1, arquivo);
  for (int i = 0; i < numBlocks; i++)
  {
    fread(&blocos[i][0], sizeof(unsigned char), blockSize, arquivo);
  }

  // Índice do primeiro inode livre.
  int inodeIndex = getFreeInode(numInodes, inodes);

  // Vetor de blocos usados - False = livre, True = usado.
  vector<bool> blocosUsados = mappingUsedBlocks(inodes, numBlocks, numInodes);

  // Quantidade de blocos necessários para armazenar o conteúdo do arquivo.
  int blocosArquivo = ceil((double)fileContent.size() / (double)blockSize);

  // Blocos livres que serão usados para armazenar o conteudo do arquivo.
  int blocosLivres[blocosArquivo];
  int contador = 0;
  for (int i = 0; i < numBlocks; i++)
  {
    if (blocosUsados[i] == false)
    {
      blocosLivres[contador] = i;
      contador++;
    }
    if (contador == blocosArquivo)
    {
      break;
    }
  }

  // Colocar o conteudo do arquivo nos blocos livres.
  int fileContentSize = fileContent.size();
  int fileContentAux = 0;
  for (int i = 0; i < blocosArquivo; i++)
  {
    for (int j = 0; j < blockSize; j++)
    {
      if (fileContentAux < fileContentSize)
      {
        blocos[blocosLivres[i]][j] = fileContent[fileContentAux];
        fileContentAux++;
      }
      else
      {
        blocos[blocosLivres[i]][j] = 0x00;
      }
    }
  }

  // Preencher o inode livre com os dados do arquivo.
  inodes[inodeIndex].IS_USED = 0x01;
  inodes[inodeIndex].IS_DIR = 0x00;
  inodes[inodeIndex].SIZE = fileContent.size();

  // Nome do arquivo, preencher com 0x00.
  string nomeArquivo = filePath.substr(filePath.find_last_of("/") + 1);
  for (int i = 0; i < 10; i++)
  {
    if (i < nomeArquivo.size())
    {
      inodes[inodeIndex].NAME[i] = nomeArquivo[i];
    }
    else
    {
      inodes[inodeIndex].NAME[i] = 0x00;
    }
  }

  // Blocos livres que serão usados para armazenar o conteudo do arquivo.
  for (int i = 0; i < blocosArquivo; i++)
  {
    inodes[inodeIndex].DIRECT_BLOCKS[i] = blocosLivres[i];

    if (i >= 0 && i < 3)
    {
      inodes[inodeIndex].DIRECT_BLOCKS[i] = blocosLivres[i];
    }
    else if (i >= 3 && i < 6)
    {
      inodes[inodeIndex].INDIRECT_BLOCKS[i - 3] = blocosLivres[i];
    }
    else
    {
      inodes[inodeIndex].DOUBLE_INDIRECT_BLOCKS[i - 6] = blocosLivres[i];
    }
  }

  // Nome do pai do arquivo/diretório.
  string nomePai = getFatherName(filePath);

  // Índice do inode do pai do arquivo/diretório.
  int inodePai = getInodeIndex(nomePai, inodes, numInodes);

  // Incrementar o tamanho do pai
  inodes[inodePai].SIZE += 1;

  // Atualizar o bloco do pai com o novo inode que foi alocado.
  for (int i = 0; i < blockSize; i++)
  {
    if (blocos[inodes[inodePai].DIRECT_BLOCKS[0]][i] == 0x00)
    {
      blocos[inodes[inodePai].DIRECT_BLOCKS[0]][i] = inodeIndex;
      break;
    }
  }

  // Mapear os blocos usados para obter o bitmap.
  blocosUsados = mappingUsedBlocks(inodes, numBlocks, numInodes);

  for (int i = 0; i < numBlocks; i++)
  {
    int index = (int)floor(i / 8.0);

    if (blocosUsados[i] == true && i < 8)
    {
      bitMap[index] |= (1 << i);
    }
  }

  // Posicionar o ponteiro após o numero de inodes e escrever o bitmap e os inodes no arquivo.
  fseek(arquivo, 3, SEEK_SET);
  fwrite(&bitMap[0], sizeof(unsigned char), bitMapSize, arquivo);
  fwrite(&inodes[0], sizeof(INODE), numInodes, arquivo);

  // Pular a raiz e escrever os blocos tratados no arquivo.
  fseek(arquivo, 1, SEEK_CUR);
  for (int i = 0; i < numBlocks; i++)
  {
    fwrite(&blocos[i][0], sizeof(unsigned char), blockSize, arquivo);
  }
}

/**
 * @brief Adiciona um novo diretório dentro do sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param arquivo arquivo aberto que contém um sistema de arquivos que simula EXT3.
 * @param dirPath caminho completo novo diretório dentro sistema de arquivos que simula EXT3.
 */
void adicionarDiretorio(FILE *arquivo, string dirPath)
{
  // blockSize: tamanho do bloco
  // numBlocks: quantidade de blocos
  // numInodes: quantidade de inodes
  // root: inode do diretório raiz
  unsigned char blockSize, numBlocks, numInodes, root;

  // Posicionamento do ponteiro no inicio do arquivo e leitura dos 3 primeiros bytes.
  fseek(arquivo, 0, SEEK_SET);
  fread(&blockSize, sizeof(unsigned char), 1, arquivo);
  fread(&numBlocks, sizeof(unsigned char), 1, arquivo);
  fread(&numInodes, sizeof(unsigned char), 1, arquivo);

  // Quantidade de bytes do mapa de bits.
  unsigned char bitMapSize = getBitMapSize(numBlocks);

  // Espaço para tratamento do mapa de bits
  vector<unsigned char> bitMap(bitMapSize);

  // Espaço para tratamento dos inodes.
  vector<INODE> inodes(numInodes);

  // Espaço para o vetor de blocos.
  vector<vector<unsigned char>> blocos(numBlocks, vector<unsigned char>(blockSize));

  // Leitura dos bytes do mapa de bits, inodes, root e blocos.
  fread(&bitMap[0], sizeof(unsigned char), bitMapSize, arquivo);
  fread(&inodes[0], sizeof(INODE), numInodes, arquivo);
  fread(&root, 1, 1, arquivo);
  for (int i = 0; i < numBlocks; i++)
  {
    fread(&blocos[i][0], sizeof(unsigned char), blockSize, arquivo);
  }

  // Índice do primeiro inode livre.
  int inodeIndex = getFreeInode(numInodes, inodes);

  // Vetor de blocos usados - False = livre, True = usado.
  vector<bool> blocosUsados = mappingUsedBlocks(inodes, numBlocks, numInodes);

  // Blocos livres que serão usados para armazenar o conteudo do arquivo.
  int blocosLivres[1];
  int contador = 0;
  for (int i = 0; i < numBlocks; i++)
  {
    if (blocosUsados[i] == false)
    {
      blocosLivres[contador] = i;
      contador++;
    }
    if (contador == 1)
    {
      break;
    }
  }

  // Preencher o inode livre com os dados do arquivo
  inodes[inodeIndex].IS_USED = 0x01;
  inodes[inodeIndex].IS_DIR = 0x01;
  inodes[inodeIndex].SIZE = 0x00;

  // Nome do arquivo, preencher com 0x00
  string nomeArquivo = dirPath.substr(dirPath.find_last_of("/") + 1);
  for (int i = 0; i < 10; i++)
  {
    if (i < nomeArquivo.size())
    {
      inodes[inodeIndex].NAME[i] = nomeArquivo[i];
    }
    else
    {
      inodes[inodeIndex].NAME[i] = 0x00;
    }
  }

  // Blocos livres que serão usados para armazenar o conteudo do arquivo
  for (int i = 0; i < 1; i++)
  {
    inodes[inodeIndex].DIRECT_BLOCKS[i] = blocosLivres[i];
  }

  // Nome do pai do arquivo/diretório.
  string nomePai = getFatherName(dirPath);

  // Índice do inode do pai do arquivo/diretório.
  int inodePai = getInodeIndex(nomePai, inodes, numInodes);

  inodes[inodePai].SIZE += 1;

  // Atualizar o bloco do pai com o novo inode que foi alocado.
  for (int i = 0; i < blockSize; i++)
  {
    if (blocos[inodes[inodePai].DIRECT_BLOCKS[0]][i] == 0x00)
    {
      blocos[inodes[inodePai].DIRECT_BLOCKS[0]][i] = inodeIndex;
      break;
    }
  }

  // Mapear os blocos usados para obter o bitmap. bitmap é um unsigned char
  blocosUsados = mappingUsedBlocks(inodes, numBlocks, numInodes);

  for (int i = 0; i < numBlocks; i++)
  {
    int index = (int)floor(i / 8.0);

    if (blocosUsados[i] == true && i < 8)
    {
      bitMap[index] |= (1 << i);
    }
  }

  // Posicionar o ponteiro após o numero de inodes e escrever o bitmap e os inodes no arquivo.
  fseek(arquivo, 3, SEEK_SET);
  fwrite(&bitMap[0], sizeof(unsigned char), bitMapSize, arquivo);
  fwrite(&inodes[0], sizeof(INODE), numInodes, arquivo);

  // Pular a raiz e escrever os blocos tratados no arquivo.
  fseek(arquivo, 1, SEEK_CUR);
  for (int i = 0; i < numBlocks; i++)
  {
    fwrite(&blocos[i][0], sizeof(unsigned char), blockSize, arquivo);
  }
}

/**
 * @brief Remove um arquivo ou diretório (recursivamente) de um sistema de arquivos que simula EXT3.
 * @param arquivo arquivo aberto que contém um sistema de arquivos que simula EXT3.
 * @param path caminho completo do arquivo ou diretório a ser removido.
 */
void remover(FILE *arquivo, string path)
{
    // remover arquivo

		// blockSize: tamanho do bloco
		// numBlocks: quantidade de blocos
		// numInodes: quantidade de inodes
		// root: inode do diretório raiz

    // Ler o superbloco
    unsigned char blockSize, numBlocks, numInodes, root;
    fseek(arquivo, 0, SEEK_SET);
    fread(&blockSize, sizeof(unsigned char), 1, arquivo);
    fread(&numBlocks, sizeof(unsigned char), 1, arquivo);
    fread(&numInodes, sizeof(unsigned char), 1, arquivo);

    // Calcular o tamanho do bitmap
    unsigned char bitMapSize = getBitMapSize(numBlocks);

    // Ler o bitmap
    vector<unsigned char> bitMap(bitMapSize);
    fread(&bitMap[0], sizeof(unsigned char), bitMapSize, arquivo);

    // Ler os inodes
    vector<INODE> inodes(numInodes);
    fread(&inodes[0], sizeof(INODE), numInodes, arquivo);

    // Ler o inode raiz
    fread(&root, 1, 1, arquivo);

    // Ler os blocos de dados
    vector<vector<unsigned char>> blocos(numBlocks, vector<unsigned char>(blockSize));
    for (int i = 0; i < numBlocks; i++)
    {
        fread(&blocos[i][0], sizeof(unsigned char), blockSize, arquivo);
    }

    // Obter o nome e o inode do arquivo ou diretório a ser removido
    string nomeRemover = getName(path);
    int inodeRemover = getInodeIndex(nomeRemover, inodes, numInodes);

    if (inodeRemover == -1)
    {
        printf("Arquivo ou diretório não encontrado!\n");
        return;
    }

    // Obter o inode do pai
    string nomePai = getFatherName(path);
    int inodePai = getInodeIndex(nomePai, inodes, numInodes);

    if (inodePai == -1)
    {
        printf("Diretório pai não encontrado!\n");
        return;
    }

    // Função recursiva para remover inodes
    function<void(int)> removerInode = [&](int inodeIndex) {
        // Se for um diretório, remover recursivamente seus filhos
        if (inodes[inodeIndex].IS_DIR == 0x01)
        {
            for (int i = 0; i < 3; i++)
            {
                unsigned char blockIdx = inodes[inodeIndex].DIRECT_BLOCKS[i];
                if (blockIdx != 0x00)
                {
                    for (int j = 0; j < blockSize; j++)
                    {
                        if (blocos[blockIdx][j] != 0x00)
                        {
                            int childInodeIndex = blocos[blockIdx][j];
                            removerInode(childInodeIndex);
                        }
                    }
                }
            }
            // Repetir para INDIRECT_BLOCKS e DOUBLE_INDIRECT_BLOCKS se necessário
            // Implementação omitida para brevidade
        }

        // Liberar os blocos usados pelo inode
        for (int i = 0; i < 3; i++)
        {
            if (inodes[inodeIndex].DIRECT_BLOCKS[i] != 0x00)
            {
                int blockIdx = inodes[inodeIndex].DIRECT_BLOCKS[i];
                // Marcar bloco como livre
                int byteIndex = blockIdx / 8;
                int bitIndex = blockIdx % 8;
                bitMap[byteIndex] &= ~(1 << bitIndex);
                // Limpar bloco
                // fill(blocos[blockIdx].begin(), blocos[blockIdx].end(), 0x00);
                // inodes[inodeIndex].DIRECT_BLOCKS[i] = 0x00;
            }
            // Repetir para INDIRECT_BLOCKS e DOUBLE_INDIRECT_BLOCKS se necessário
            // Implementação omitida para brevidade
        }

        // Limpar inode
        inodes[inodeIndex].IS_USED = 0x00;
        // inodes[inodeIndex].IS_DIR = 0x00;
        // inodes[inodeIndex].SIZE = 0x00;
        // memset(inodes[inodeIndex].NAME, 0x00, 10);
        // memset(inodes[inodeIndex].DIRECT_BLOCKS, 0x00, 3);
        // memset(inodes[inodeIndex].INDIRECT_BLOCKS, 0x00, 3);
        // memset(inodes[inodeIndex].DOUBLE_INDIRECT_BLOCKS, 0x00, 3);

    };

    // Remover o inode alvo
    removerInode(inodeRemover);

    // Atualizar o diretório pai
    bool found = false;
    for (int i = 0; i < 3; i++)
    {
        unsigned char blockIdx = inodes[inodePai].DIRECT_BLOCKS[i];
        if (blockIdx != 0x00)
        {
            for (int j = 0; j < blockSize; j++)
            {
                if (blocos[blockIdx][j] == inodeRemover)
                {
                    // Remover referência ao inode removido
                    blocos[blockIdx][j] = 0x00;
                    found = true;
                    break;
                }
            }
            if (found)
                break;
        }
    }

    if (!found)
    {
        printf("Referência ao inode não encontrada no diretório pai.\n");
    }
    // Decrementar o tamanho do diretório pai
    inodes[inodePai].SIZE -= 1;

    // Reescrever os dados no arquivo
    fseek(arquivo, 3, SEEK_SET);
    fwrite(&bitMap[0], sizeof(unsigned char), bitMapSize, arquivo);
    fwrite(&inodes[0], sizeof(INODE), numInodes, arquivo);
    // fwrite(&root, sizeof(unsigned char), 1, arquivo);
    // for (int i = 0; i < numBlocks; i++)
    // {
    //     fwrite(&blocos[i][0], sizeof(unsigned char), blockSize, arquivo);
    // }
}

/**
 * @brief Move um arquivo ou diretório em um sistema de arquivos que simula EXT3.
 * @param arquivo arquivo aberto que contém um sistema de arquivos que simula EXT3.
 * @param oldPath caminho completo do arquivo ou diretório a ser movido.
 * @param newPath novo caminho completo do arquivo ou diretório.
 */
void mover(FILE *arquivo, string oldPath, string newPath)
{
    // Ler o superbloco
    unsigned char blockSize, numBlocks, numInodes, root;
    fseek(arquivo, 0, SEEK_SET);
    fread(&blockSize, sizeof(unsigned char), 1, arquivo);
    fread(&numBlocks, sizeof(unsigned char), 1, arquivo);
    fread(&numInodes, sizeof(unsigned char), 1, arquivo);

    // Calcular o tamanho do bitmap
    unsigned char bitMapSize = getBitMapSize(numBlocks);

    // Ler o bitmap
    vector<unsigned char> bitMap(bitMapSize);
    fread(&bitMap[0], sizeof(unsigned char), bitMapSize, arquivo);

    // Ler os inodes
    vector<INODE> inodes(numInodes);
    fread(&inodes[0], sizeof(INODE), numInodes, arquivo);

    // Ler o inode raiz
    fread(&root, 1, 1, arquivo);

    // Ler os blocos de dados
    vector<vector<unsigned char>> blocos(numBlocks, vector<unsigned char>(blockSize));
    for (int i = 0; i < numBlocks; i++)
    {
        fread(&blocos[i][0], sizeof(unsigned char), blockSize, arquivo);
    }

    // Obter o nome e o inode do arquivo ou diretório a ser movido
    string nomeMover = getName(oldPath);
    int inodeMover = getInodeIndex(nomeMover, inodes, numInodes);

    if (inodeMover == -1)
    {
        printf("Arquivo ou diretório não encontrado!\n");
        return;
    }

    // Obter o inode do pai antigo
    string nomePaiAntigo = getFatherName(oldPath);
    int inodePaiAntigo = getInodeIndex(nomePaiAntigo, inodes, numInodes);

    if (inodePaiAntigo == -1)
    {
        printf("Diretório pai antigo não encontrado!\n");
        return;
    }

    // Obter o inode do novo pai
    string nomePaiNovo = getFatherName(newPath);
    int inodePaiNovo = getInodeIndex(nomePaiNovo, inodes, numInodes);

    if (inodePaiNovo == -1)
    {
        printf("Diretório pai novo não encontrado!\n");
        return;
    }

    // Atualizar o nome do inode
    string novoNome = getName(newPath);
    memset(inodes[inodeMover].NAME, 0x00, 10);
    strncpy((char *)inodes[inodeMover].NAME, novoNome.c_str(), 10);

    // Remover a referência no diretório pai antigo
    bool found = false;
    for (int i = 0; i < 3; i++)
    {
        unsigned char blockIdxPaiAntigo = inodes[inodePaiAntigo].DIRECT_BLOCKS[i];
        if (blockIdxPaiAntigo != 0x00)
        {
            for (int j = 0; j < blockSize; j++)
            {
                if (blocos[blockIdxPaiAntigo][j] == inodeMover)
                {
                    blocos[blockIdxPaiAntigo][j] = 0x00;
                    found = true;
                    break;
                }
            }
            if (found)
                break;
        }
    }
    if (!found)
    {
        printf("Referência ao inode não encontrada no diretório pai antigo.\n");
    }
    // Decrementar o tamanho do diretório pai antigo
    inodes[inodePaiAntigo].SIZE -= 1;

    // Adicionar a referência no novo diretório pai
    bool added = false;
    for (int i = 0; i < 3; i++)
    {
        unsigned char blockIdxPaiNovo = inodes[inodePaiNovo].DIRECT_BLOCKS[i];
        if (blockIdxPaiNovo == 0x00)
        {
            // Encontrar um bloco livre
            for (int b = 1; b < numBlocks; b++)
            {
                int byteIndex = b / 8;
                int bitIndex = b % 8;
                if (!(bitMap[byteIndex] & (1 << bitIndex)))
                {
                    // Bloco livre encontrado
                    bitMap[byteIndex] |= (1 << bitIndex);
                    inodes[inodePaiNovo].DIRECT_BLOCKS[i] = b;
                    blockIdxPaiNovo = b;
                    fill(blocos[blockIdxPaiNovo].begin(), blocos[blockIdxPaiNovo].end(), 0x00);
                    break;
                }
            }
            if (blockIdxPaiNovo == 0x00)
            {
                printf("Não foi possível mover: sem blocos livres.\n");
                return;
            }
        }
        for (int j = 0; j < blockSize; j++)
        {
            if (blocos[blockIdxPaiNovo][j] == 0x00)
            {
                blocos[blockIdxPaiNovo][j] = inodeMover;
                added = true;
                break;
            }
        }
        if (added)
            break;
    }
    if (!added)
    {
        printf("Não foi possível mover: bloco do diretório pai cheio.\n");
        return;
    }
    // Incrementar o tamanho do novo diretório pai
    inodes[inodePaiNovo].SIZE += 1;

    // Reescrever os dados no arquivo
    fseek(arquivo, 3, SEEK_SET);
    fwrite(&bitMap[0], sizeof(unsigned char), bitMapSize, arquivo);
    fwrite(&inodes[0], sizeof(INODE), numInodes, arquivo);
    fwrite(&root, sizeof(unsigned char), 1, arquivo);
    // for (int i = 0; i < numBlocks; i++)
    // {
    //     fwrite(&blocos[i][0], sizeof(unsigned char), blockSize, arquivo);
    // }
}

#endif /* auxFunction_hpp */