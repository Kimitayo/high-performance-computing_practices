/**
 * TAREFA 1 - ENUNCIADO
 * Implemente duas versões da multiplicação de matriz por vetor (MxV) em C: uma com acesso à matriz por linhas (linha externa, coluna interna) e outra por colunas (coluna externa, linha interna). Meça o tempo de execução de cada versão com uma função apropriada e execute testes com diferentes tamanhos de matriz. Identifique, a partir de que tamanho, os tempos passam a divergir significativamente e explique por que isso ocorre, relacionando suas observações ao uso da memória cache e ao padrão de acesso à memória.
 */

/**
 * EXPLICAÇÃO
 * Na programação em C, trabalhamos com matrizes bidimensionais, sendo que na realidade, na memória RAM o formato é unidimensional, em que cada trecho de linha da matriz é colocada em sequência. 
 * A CPU trabalha muito rapidamente, mas a memória RAM é muito mais lenta ao fornecer o armazenamento para a CPU. Dessa maneira, existe a memória cache, que é muito pequena, mas muito mais rápida que a memória RAM. 
 * - Localidade espacial: quando é pedido um elemento da matriz[0][0], o computador entrega a linha inteira para o cache, assumindo que serão usadas também.
 * - Localidade temporal: o computador presume que se uma variável foi acessada agora, tem uma grande chance dela ser acessada novamente daqui a poucos milissegundos.
 */

 /**
  * EXPLICAÇÃO DA TAREFA
  * Analisar o tempo de execução entre:
  * - computador não indo direto na memória cache -> a CPU vai trabalhar diretamente com a memória RAM, pulando de linha em linha ao invés de dispor uma linha contínua a ser trabalhada na cache.
  * - computador diretamente na memória cache -> a CPU trabalhará diretamente com a m. cache, com acesso a linha inteira em colunas diferentes.
  */

  /**
   * COMO RESOLVER?
   * 1º) Criar um vetor 1D de tamanho n*n que será a minha matriz -> melhor que criar da forma tradicional, pois será mais complicado para essa atividade, e assim estou usando malloc alugando um bom espaço na memória.
   * 2º) Criar um vetor 1D normal de tamnho n -> para alocar o vetor entrada
   * 3º) Criar um vetor 1D normal de tamanho n -> para alocar as multiplicações.
   * 4º) Fazer testes com diversos valores para n e calcular o tempo de execução de cada um.
   */


#include <stdio.h> 
#include <stdlib.h> 
#include <time.h>  

int main() {
    int tamanhos[] = {100, 500, 1000, 2000, 4000, 8000};
    int qtd_tamanhos = 6;

    for (int t=0; t<qtd_tamanhos; t++) {
        int N = tamanhos[t];

        // Criar a matriz e os vetores
        double *matriz;
        double *vetor_x;
        double *vetor_y;

        matriz = (double *) malloc( (N*N)*sizeof(double) );
        vetor_x = (double *) malloc( (N)*sizeof(double) );
        vetor_y = (double *) malloc( (N)*sizeof(double) );

        if (matriz == NULL || vetor_x == NULL || vetor_y == NULL) {
            printf("Erro: Faltou memória!\n");
            return 1;
        } 

        // Preencher a matriz com 1
        for (int i=0; i<N; i++) {
            for (int j=0; j<N; j++) {
                matriz[(i*N)+j] = 1;
            }
        }

        // Preencher o vetor_x com 1 -> já que esses valores serão multiplicados, para manter o valor N
        // Preencher o vetor_y com 0 -> malloc sempre deixa valores dentro, por isso colocar 0
        for (int i=0; i<N; i++) {
            vetor_x[i] = 1;
            vetor_y[i] = 0;
        }

        // Utlizar 10 repetições para pegar a média desses valores contando com as variações de cada laço
        double acumulador1 = 0;
        for (int rep=0; rep<10; rep++) {
            for (int l=0; l<N; l++) {
                vetor_y[l] = 0;
            }
            // Iniciar o relógio
            clock_t inicio_linhas = clock();

            // Versão por linhas
            for (int i=0; i<N; i++) { // i=0
                for (int j=0; j<N; j++) { // j=0
                    double mult = vetor_x[j] * matriz[(i*N)+j];
                    vetor_y[i] += mult;
                }
            }

            clock_t fim_linhas = clock();
            double tempo_linhas = ((double)(fim_linhas - inicio_linhas) * 1000) / CLOCKS_PER_SEC;
            // printf("Tempo de acesso por LINHAS: %f ms\n", tempo_linhas);

            acumulador1 += tempo_linhas;
        }

        // Versão por colunas
        double acumulador2 = 0;
        for (int rep=0; rep<10; rep++) {
            for (int l=0; l<N; l++) {
                vetor_y[l] = 0;
            }
            // Iniciar o relógio
            clock_t inicio_colunas = clock();

            // Versão por colunas
            for (int j=0; j<N; j++) { 
                for (int i=0; i<N; i++) { 
                    double mult = vetor_x[j] * matriz[(i*N)+j];
                    vetor_y[i] += mult;
                }
            }

            clock_t fim_colunas = clock();
            double tempo_colunas = ((double)(fim_colunas - inicio_colunas) * 1000) / CLOCKS_PER_SEC;

            acumulador2 += tempo_colunas;
        }


        double media_tempo_linha = acumulador1/10;
        double media_tempo_coluna = acumulador2/10;
        double diferenca = media_tempo_coluna - media_tempo_linha;
        printf("Tempo de acesso por LINHAS TOTAL em N=%d: %f ms\n", N, media_tempo_linha);
        printf("Tempo de acesso por COLUNAS TOTAL em N=%d: %f ms\n", N, media_tempo_coluna);
        printf("DIFERENCA EM N=%d: %f ms\n\n\n", N, diferenca);


        free(matriz);
        free(vetor_x);
        free(vetor_y);
    }
    
    

    return 0; 
}


/**
 * EXPLICAÇÃO DO RESULTADO
 * Para valores pequenos de N, os tempos não diferem tão significamente, sendo que quanto mais expressivo esse tamanho, mais perceptível é essa diferença no tempo já que por linhas o acesso acontece mais rapidamente, percorrendi posições consecutivas na memória, aproveitando a linha do ache trazida pela CPU; já no acesso por colunas, ele faz saltos de tamanho N, desperdiçando a funciinalidade do cache.
 */