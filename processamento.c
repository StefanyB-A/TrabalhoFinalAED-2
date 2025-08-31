#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "structs.h"
#include "processamento.h"
#include "arvoreAVL.h"
#include "arvoreBB.h"
#include "buscaB.h"

void limparPalavra(char* palavra_original, char* palavra_limpa) {
    int j = 0;
    int len = strlen(palavra_original);
    
    for (int i = 0; i < len; i++) {
        if (isalpha(palavra_original[i])) {
            palavra_limpa[j] = tolower(palavra_original[i]);
            j++;
        }
    }
    palavra_limpa[j] = '\0';
}

int palavraValida(char* palavra) {
    return strlen(palavra) > 3;
}

int extrairInfoMusica(FILE* arquivo, InfoMusica* info) {
    char linha[500];
    int info_extraida = 0;
    
    //LÃª as primeiras linhas para extrair nome da mÃºsica e compositor
    while (fgets(linha, sizeof(linha), arquivo) && info_extraida < 2) {
        linha[strcspn(linha, "\n")] = 0;
        
        if (strlen(linha) > 0) {
            if (info_extraida == 0) {
                strncpy(info->nome_musica, linha, 99);
                info->nome_musica[99] = '\0';
                info_extraida++;
            } else if (info_extraida == 1) {
                strncpy(info->compositor, linha, 99);
                info->compositor[99] = '\0';
                info_extraida++;
            }
        }
    }
    
    return info_extraida == 2;
}

char* lerLetraCompleta(FILE* arquivo, int* tamanho) {
    fseek(arquivo, 0, SEEK_END);
    long tamanho_arquivo = ftell(arquivo);
    rewind(arquivo);
    
    char* conteudo = malloc(tamanho_arquivo + 1);
    if (!conteudo) {
        return NULL;
    }
    
    size_t bytes_lidos = fread(conteudo, 1, tamanho_arquivo, arquivo);
    conteudo[bytes_lidos] = '\0';
    *tamanho = bytes_lidos;
    
    return conteudo;
}

void encontrarEstrofeComPalavra(char* letra_completa, char* palavra_procurada, char* estrofe_resultado) {
    char* aux = letra_completa;
    char* inicio_estrofe = aux;
    int chars_copiados = 0;
    
    while (*aux != '\0') {
        if (*aux == '\n' && *(aux + 1) == '\n') {
            inicio_estrofe = aux + 2;
            aux += 2;
            continue;
        }
        
        if (strncmp(aux, palavra_procurada, strlen(palavra_procurada)) == 0) {
            char* inicio_copia = inicio_estrofe;
            chars_copiados = 0;
            
            while (*inicio_copia != '\0' && chars_copiados < 100) {
                if (*inicio_copia == '\n' && *(inicio_copia + 1) == '\n') {
                    break;
                }
                estrofe_resultado[chars_copiados] = *inicio_copia;
                inicio_copia++;
                chars_copiados++;
            }
            estrofe_resultado[chars_copiados] = '\0';
            return;
        }
        aux++;
    }
    
    strncpy(estrofe_resultado, letra_completa, 100);
    estrofe_resultado[100] = '\0';
}

int lerArquivo(char* nome_arquivo, VetorBB* vetor, NoABB** raiz_bst, NoAVL** raiz_avl, NoAVL** raiz_freq) {
    
    FILE* arquivo = fopen(nome_arquivo, "r");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo: %s\n", nome_arquivo);
        return 0;
    }
    
    InfoMusica info_musica;
    
    if (!extrairInfoMusica(arquivo, &info_musica)) {
        printf("Erro ao extrair informaÃ§Ãµes da mÃºsica do arquivo: %s\n", nome_arquivo);
        fclose(arquivo);
        return 0;
    }
    
    fseek(arquivo, 0, SEEK_SET);
    info_musica.letra_completa = lerLetraCompleta(arquivo, &info_musica.tamanho_letra);
    fclose(arquivo);
    
    if (!info_musica.letra_completa) {
        printf("Erro ao ler a letra da mÃºsica: %s\n", nome_arquivo);
        return 0;
    }
    
    
    ContadorPalavra contadores[1000]; //Assumindo mÃ¡ximo de 1000 palavras Ãºnicas por mÃºsica
    int total_palavras_unicas = 0;
    
    char* token;
    char* copia_letra = malloc(strlen(info_musica.letra_completa) + 1);
    strcpy(copia_letra, info_musica.letra_completa);
    
    token = strtok(copia_letra, " \t\n\r.,!?;:()[]{}\"'-");
    
    while (token != NULL) {
        char palavra_limpa[100];
        limparPalavra(token, palavra_limpa);

        if (palavraValida(palavra_limpa)) {
            //Procura se a palavra jÃ¡ foi contada nesta mÃºsica
            int encontrada = 0;
            for (int i = 0; i < total_palavras_unicas; i++) {
                if (strcmp(contadores[i].palavra, palavra_limpa) == 0) {
                    contadores[i].frequencia++;
                    encontrada = 1;
                    break;
                }
            }
            
            //Se nÃ£o foi encontrada, adiciona nova entrada
            if (!encontrada && total_palavras_unicas < 1000) {
                strcpy(contadores[total_palavras_unicas].palavra, palavra_limpa);
                contadores[total_palavras_unicas].frequencia = 1;
                encontrarEstrofeComPalavra(info_musica.letra_completa, palavra_limpa, contadores[total_palavras_unicas].trecho);
                total_palavras_unicas++;
            }
        }
        
        token = strtok(NULL, " \t\n\r.,!?;:()[]{}\"'-");
    }
    
    //Insere cada palavra Ãºnica nas estruturas de dados
    for (int i = 0; i < total_palavras_unicas; i++) {
        PalavraInfo entrada;
        strcpy(entrada.palavra, contadores[i].palavra);
        strcpy(entrada.dados.nome, info_musica.nome_musica);
        strcpy(entrada.dados.compositor, info_musica.compositor);
        strcpy(entrada.dados.trecho, contadores[i].trecho);
        entrada.dados.freq_palavra = contadores[i].frequencia;
        entrada.freq = contadores[i].frequencia; //SerÃ¡ atualizado pelas funÃ§Ãµes de inserÃ§Ã£o
        
        //Chama as funÃ§Ãµes de inserÃ§Ã£o
        clock_t inicio = clock();
        inserirOrdenado(vetor, entrada);
        clock_t fim = clock();
        double tempo = ((double)(fim - inicio)) / CLOCKS_PER_SEC;
        printf("Tempo de inserÃ§Ã£o no vetor: %.6f segundos\n", tempo);

        inicio = clock();
        *raiz_bst = insereABB(*raiz_bst, entrada);
        fim = clock();
        tempo = ((double)(fim - inicio)) / CLOCKS_PER_SEC;
        printf("Tempo de inserÃ§Ã£o na Ã¡rvore BST: %.6f segundos\n", tempo);

        inicio = clock();
        *raiz_avl = insereAVL(*raiz_avl, entrada);
        fim = clock();
        tempo = ((double)(fim - inicio)) / CLOCKS_PER_SEC;
        printf("Tempo de inserÃ§Ã£o na Ã¡rvore AVL: %.6f segundos\n", tempo);

        inicio = clock();
        *raiz_freq = insereAVLFreq(*raiz_freq, entrada);
        fim = clock();
        tempo = ((double)(fim - inicio)) / CLOCKS_PER_SEC;
        printf("Tempo de inserÃ§Ã£o na Ã¡rvore AVL de frequÃªncia: %.6f segundos\n", tempo);
    }
    
    
    free(info_musica.letra_completa);
    free(copia_letra);
    
    printf("Arquivo processado com sucesso: %s\n", nome_arquivo);
    printf("Total de palavras Ãºnicas encontradas: %d\n", total_palavras_unicas);
    
    return 1;
}