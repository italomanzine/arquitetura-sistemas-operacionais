#include "auxFunction.hpp"

/**
 * @brief Inicializa um sistema de arquivos que simula EXT3
 * @param fsFileName nome do arquivo que contém sistema de arquivos que simula EXT3 (caminho do arquivo no sistema de arquivos local)
 * @param blockSize tamanho em bytes do bloco
 * @param numBlocks quantidade de blocos
 * @param numInodes quantidade de inodes
 */
void initFs(string fsFileName, int blockSize, int numBlocks, int numInodes)
{
	// Arquivo a ser aberto no modo wb+ (escrita e leitura)
	FILE *arquivo = fopen(fsFileName.c_str(), "wb+");
	if (arquivo == NULL)
	{
		printf("Error opening file!\n");
		exit(1);
	}

	inicializar(arquivo, blockSize, numBlocks, numInodes);

	// Fechando o arquivo
	fclose(arquivo);
}

/**
 * @brief Adiciona um novo arquivo dentro do sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param fsFileName arquivo que contém um sistema de arquivos que simula EXT3.
 * @param filePath caminho completo novo arquivo dentro sistema de arquivos que simula EXT3.
 * @param fileContent conteúdo do novo arquivo
 */
void addFile(string fsFileName, string filePath, string fileContent)
{
	// Arquivo a ser aberto no modo r+
	FILE *arquivo = fopen(fsFileName.c_str(), "r+");
	if (arquivo == NULL)
	{
		printf("Error opening file!\n");
		exit(1);
	}

	adicionarArquivo(arquivo, filePath, fileContent);

	fclose(arquivo);
}

/**
 * @brief Adiciona um novo diretório dentro do sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
 * @param dirPath caminho completo novo diretório dentro sistema de arquivos que simula EXT3.
 */
void addDir(string fsFileName, string dirPath)
{
	// Arquivo a ser aberto no modo r+
	FILE *arquivo = fopen(fsFileName.c_str(), "r+");
	if (arquivo == NULL)
	{
		printf("Error opening file!\n");
		exit(1);
	}

	adicionarDiretorio(arquivo, dirPath);

	fclose(arquivo);
}

/**
 * @brief Remove um arquivo ou diretório (recursivamente) de um sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
 * @param path caminho completo do arquivo ou diretório a ser removido.
 */
void remove(string fsFileName, string path)
{
    // Arquivo a ser aberto no modo r+
    FILE *arquivo = fopen(fsFileName.c_str(), "r+");
    if (arquivo == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }

    remover(arquivo, path);

    fclose(arquivo);
}

/**
 * @brief Move um arquivo ou diretório em um sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
 * @param oldPath caminho completo do arquivo ou diretório a ser movido.
 * @param newPath novo caminho completo do arquivo ou diretório.
 */
void move(string fsFileName, string oldPath, string newPath)
{
    // Arquivo a ser aberto no modo r+
    FILE *arquivo = fopen(fsFileName.c_str(), "r+");
    if (arquivo == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }

    mover(arquivo, oldPath, newPath);

    fclose(arquivo);
}