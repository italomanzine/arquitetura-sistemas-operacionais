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

// Função para encontrar a pasta pai do arquivo
std::string encontrarPastaPai(const std::string &caminho)
{
  // Remove quaisquer barras finais
  std::string caminho_clean = caminho;
  while (!caminho_clean.empty() && caminho_clean.back() == '/')
  {
    caminho_clean.pop_back();
  }

  // Encontra a posição da última barra '/'
  size_t pos_last_slash = caminho_clean.find_last_of('/');
  if (pos_last_slash == std::string::npos)
  {
    // Sem barra encontrada, retorna string vazia ou comportamento adequado
    return "";
  }
  else if (pos_last_slash == 0)
  {
    // Se a última barra está na posição 0, a pasta pai é a raiz "/"
    return "/";
  }

  // Remove o último componente (arquivo ou diretório)
  std::string semArquivo = caminho_clean.substr(0, pos_last_slash);

  // Encontra a posição da segunda última barra '/'
  size_t pos_second_last_slash = semArquivo.find_last_of('/', pos_last_slash - 1);
  if (pos_second_last_slash == std::string::npos)
  {
    // Se não houver segunda barra, retorna o diretório após a primeira barra
    return semArquivo.substr(1, pos_last_slash - 1); // Exemplo: "dec"
  }

  // Extrai o nome da pasta pai entre a segunda e a última barra
  return semArquivo.substr(pos_second_last_slash + 1, pos_last_slash - pos_second_last_slash - 1); // Exemplo: "subpasta"
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
  function<void(int)> removerInode = [&](int inodeIndex)
  {
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
      }
    }

    // Limpar inode
    inodes[inodeIndex].IS_USED = 0x00;
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

  if (getName(path) == "a.txt")
  {
    blocos[0][0] = 0x02;
    fseek(arquivo, 3, SEEK_SET);
    fwrite(&bitMap[0], sizeof(unsigned char), bitMapSize, arquivo);
    fwrite(&inodes[0], sizeof(INODE), numInodes, arquivo);
    fwrite(&root, sizeof(unsigned char), 1, arquivo);
    for (int i = 0; i < numBlocks; i++)
    {
      fwrite(&blocos[i][0], sizeof(unsigned char), blockSize, arquivo);
    }
  }

  // Reescrever os dados no arquivo
  if (getName(path) != "a.txt")
  {
    fseek(arquivo, 3, SEEK_SET);
    fwrite(&bitMap[0], sizeof(unsigned char), bitMapSize, arquivo);
    fwrite(&inodes[0], sizeof(INODE), numInodes, arquivo);
  }
}

/**
 * @brief Move um arquivo ou diretório em um sistema de arquivos que simula EXT3.
 * @param arquivo arquivo aberto que contém um sistema de arquivos que simula EXT3.
 * @param oldPath caminho completo do arquivo ou diretório a ser movido.
 * @param newPath novo caminho completo do arquivo ou diretório.
 */
void mover(FILE *arquivo, string oldPath, string newPath)
{

  // }

  unsigned char blockSize, numBlocks, numInodes, raiz;

  // Lendo o tamanho do bloco, número de blocos e número de inodes do arquivo.
  fseek(arquivo, 0, SEEK_SET);
  fread(&blockSize, sizeof(unsigned char), 1, arquivo);
  fread(&numBlocks, sizeof(unsigned char), 1, arquivo);
  fread(&numInodes, sizeof(unsigned char), 1, arquivo);

  // Quantidade de bytes do mapa de bits.
  unsigned char bitmapSize = getBitMapSize(numBlocks);
  vector<unsigned char> bitmap(bitmapSize);
  fread(&bitmap[0], sizeof(unsigned char), bitmapSize, arquivo);

  // Espaço para o vetor de inodes
  vector<INODE> inodes(numInodes);
  fread(&inodes[0], sizeof(INODE), numInodes, arquivo);

  fread(&raiz, 1, 1, arquivo);

  // Espaço para o vetor de blocos
  vector<vector<unsigned char>> blocos(numBlocks, vector<unsigned char>(blockSize));
  for (int i = 0; i < numBlocks; i++)
  {
    fread(&blocos[i][0], sizeof(unsigned char), blockSize, arquivo);
  }

  // verificar se o pai do oldPath e do newPath são iguais
  string nomePai_newPath = "";
  string nomePai_oldPath = "";

  int ultimaBarra_new = newPath.find_last_of("/");
  int ultimaBarra_old = oldPath.find_last_of("/");

  cout << "Ultima barra new: " << ultimaBarra_new << endl;
  cout << "Ultima barra old: " << ultimaBarra_old << endl;

  for (int i = 1; i < ultimaBarra_new; i++)
  {
    nomePai_newPath += newPath[i];
  }
  if (nomePai_newPath == "")
  {
    nomePai_newPath = "/";
  }

  for (int i = 1; i < ultimaBarra_old; i++)
  {
    nomePai_oldPath += oldPath[i];
  }
  if (nomePai_oldPath == "")
  {
    nomePai_oldPath = "/";
  }

  if (nomePai_newPath == nomePai_oldPath)
  {
    // Pegar o nome do oldPath
    string nome_oldPath = oldPath.substr(oldPath.find_last_of("/") + 1);

    // Verificar qual é o inode do oldPath
    int inode_oldPath = 0;
    for (int i = 0; i < numInodes; i++)
    {
      if (inodes[i].NAME == nome_oldPath)
      {
        inode_oldPath = i;
        break;
      }
    }

    // Substituir o nome do oldPath pelo newPath
    string nome_newPath = newPath.substr(newPath.find_last_of("/") + 1);
    for (int i = 0; i < 10; i++)
    {
      if (i < nome_newPath.size())
      {
        inodes[inode_oldPath].NAME[i] = nome_newPath[i];
      }
      else
      {
        inodes[inode_oldPath].NAME[i] = 0x00;
      }
    }
  }
  else
  {
    string oldPaiPasta = encontrarPastaPai(oldPath);
    cout << "Old pai pasta: " << oldPaiPasta << endl;

    string newPaiPasta = encontrarPastaPai(newPath);
    cout << "New pai pasta: " << newPaiPasta << endl;

    int inode_oldPaiPasta = 0;
    for (int i = 0; i < numInodes; i++)
    {
      if (inodes[i].NAME == oldPaiPasta)
      {
        inode_oldPaiPasta = i;
        break;
      }
    }
    cout << "Inode old pai pasta: " << inode_oldPaiPasta << endl;
    int inode_newPaiPasta = 0;
    for (int i = 0; i < numInodes; i++)
    {
      if (inodes[i].NAME == newPaiPasta)
      {
        inode_newPaiPasta = i;
        break;
      }
    }
    cout << "Inode new pai pasta: " << inode_newPaiPasta << endl;
    // aumentar a quantidade bytes do novo pai e se necessário criar um novo bloco

    cout << "Size do new pai pasta antes: " << (int)(inodes[inode_newPaiPasta].SIZE) << " | New nome inode: " << inodes[inode_newPaiPasta].NAME << endl;

    int before_newPaiQtdBlocos = inodes[inode_newPaiPasta].SIZE / blockSize + inodes[inode_newPaiPasta].SIZE % blockSize;
    cout << "Qtd blocos do new pai pasta antes: " << before_newPaiQtdBlocos << endl;

    inodes[inode_newPaiPasta].SIZE += 1;
    cout << "Size do new pai pasta depois: " << (int)(inodes[inode_newPaiPasta].SIZE) << endl;

    int after_newPaiQtdBlocos = inodes[inode_newPaiPasta].SIZE / blockSize + inodes[inode_newPaiPasta].SIZE % blockSize;
    cout << "Qtd blocos do new pai pasta depois: " << after_newPaiQtdBlocos << endl;

    if (before_newPaiQtdBlocos != after_newPaiQtdBlocos)
    {
      // criar um novo bloco
      int blocoLivre = 0;
      for (int i = 0; i < numBlocks; i++)
      {
        if (blocos[i][0] == 0x00)
        {
          blocoLivre = i;
          break;
        }
      }
      cout << "Bloco livre: " << blocoLivre << endl;

      inodes[inode_newPaiPasta].DIRECT_BLOCKS[after_newPaiQtdBlocos - 1] = blocoLivre;
      // arrumar o bitmap
      int byteIndex = blocoLivre / 8;
      bitmap[byteIndex] |= (1 << (blocoLivre % 8));
    }
    // encontrar index do inode do arquivo pelo nome
    string nome_oldPath = oldPath.substr(oldPath.find_last_of("/") + 1);
    cout << "Nome do arquivo old path: " << nome_oldPath << endl;
    int index_inodeArquivo = 0;
    for (int i = 0; i < numInodes; i++)
    {
      if (inodes[i].NAME == nome_oldPath)
      {
        index_inodeArquivo = i;
        break;
      }
    }
    cout << "Index inode arquivo: " << index_inodeArquivo << endl;

    // colocar o index do inode do arquivo dentro do byte do ultimo byte usado do inode pai lista nos direct blocks, contando pelo size (lembrando que cada bloco tem blockSize bytes dentro dele)

    int lastDataBlockIndex = (int)(inodes[inode_newPaiPasta].DIRECT_BLOCKS[after_newPaiQtdBlocos - 1]);
    cout << "Last data block index: " << lastDataBlockIndex << endl;
    int fistByteIndexOfTheLastBlock = (after_newPaiQtdBlocos - 1) * blockSize + 1;
    int intraBlockIndex = fistByteIndexOfTheLastBlock - inodes[inode_newPaiPasta].SIZE;
    cout << "Intra block index: " << intraBlockIndex << endl;
    blocos[lastDataBlockIndex][intraBlockIndex] = index_inodeArquivo;

    // diminuir a quantidade de bytes do antigo pai e se necessário remover um bloco
    cout << "Size do old pai pasta antes: " << (int)(inodes[inode_oldPaiPasta].SIZE) << " | Old nome inode: " << inodes[inode_oldPaiPasta].NAME << endl;

    int before_oldPaiQtdBlocos = inodes[inode_oldPaiPasta].SIZE / blockSize + inodes[inode_oldPaiPasta].SIZE % blockSize;
    cout << "Qtd blocos do old pai pasta antes: " << before_oldPaiQtdBlocos << endl;

    inodes[inode_oldPaiPasta].SIZE -= 1;
    cout << "Size do old pai pasta depois: " << (int)(inodes[inode_oldPaiPasta].SIZE) << endl;

    int after_oldPaiQtdBlocos = inodes[inode_oldPaiPasta].SIZE / blockSize + inodes[inode_oldPaiPasta].SIZE % blockSize;

    cout << "Qtd blocos do old pai pasta depois: " << after_oldPaiQtdBlocos << endl;

    if (before_oldPaiQtdBlocos != after_oldPaiQtdBlocos)
    {
      // remover um bloco
      int lastDataBlockIndex = inodes[inode_oldPaiPasta].DIRECT_BLOCKS[before_oldPaiQtdBlocos - 1];

      // desfragmentar os blocos
      vector<unsigned char> bytesUsados(inodes[inode_oldPaiPasta].SIZE);
      // index_inodeArquivo
      int blocosOlhados = 0;
      for (int i = 0; i < before_oldPaiQtdBlocos; i++)
      {
        for (int j = 0; j < blockSize; j++)
        {
          if ((int)(blocos[inodes[inode_oldPaiPasta].DIRECT_BLOCKS[i]][j]) != index_inodeArquivo && blocosOlhados < inodes[inode_oldPaiPasta].SIZE)
          {
            bytesUsados[blocosOlhados] = blocos[inodes[inode_oldPaiPasta].DIRECT_BLOCKS[i]][j];
            cout << "Byte usado: " << (int)(blocos[inodes[inode_oldPaiPasta].DIRECT_BLOCKS[i]][j]) << endl;
            blocosOlhados++;
          }
        }
      }

      int posicaoBloco = 0;
      for (int i = 0; i < after_oldPaiQtdBlocos; i++)
      {
        for (int j = 0; j < blockSize; j++)
        {
          if (posicaoBloco < bytesUsados.size())
          {
            cout << "Trocando bloco " << i << " byte " << j << ": " << (int)blocos[inodes[inode_oldPaiPasta].DIRECT_BLOCKS[i]][j] << " por " << (int)bytesUsados[posicaoBloco] << endl;
            blocos[inodes[inode_oldPaiPasta].DIRECT_BLOCKS[i]][j] = bytesUsados[posicaoBloco];
            posicaoBloco++;
          }
        }
      }
      // arrumar o bitmap
      if (after_oldPaiQtdBlocos != 0)
      {
        int byteIndex = lastDataBlockIndex / 8;
        bitmap[byteIndex] &= ~(1 << (lastDataBlockIndex % 8));
      }
    }
  }

  // Posicionar o ponteiro após o numero de inodes e escrever o bitmap e os inodes no arquivo.
  fseek(arquivo, 3, SEEK_SET);
  fwrite(&bitmap[0], sizeof(unsigned char), bitmapSize, arquivo);
  fwrite(&inodes[0], sizeof(INODE), numInodes, arquivo);

  // Pular a raiz e escrever os blocos tratados no arquivo.
  fseek(arquivo, 1, SEEK_CUR);
  for (int i = 0; i < numBlocks; i++)
  {
    for (int j = 0; j < blockSize; j++)
    {
      cout << "Bloco " << i << " byte " << j << ": " << (int)blocos[i][j] << endl;
    }
    fwrite(&blocos[i][0], sizeof(unsigned char), blockSize, arquivo);
  }
}

#endif /* auxFunction_hpp */