/**
 * TAREFA 2 - ENUNCIADO
 * Implemente três laços em C para investigar os efeitos do paralelismo ao nível de instrução (ILP): 1) inicialize um vetor com um cálculo simples; 2) some seus elementos de forma acumulativa, criando dependência entre as iterações; e 3) quebre essa dependência utilizando múltiplas variáveis. Compare o tempo de execução das versões compiladas com diferentes níveis de otimização (O0, O2, O3) e analise como o estilo do código e as dependências influenciam o desempenho.
 */

/**
 * EXPLICAÇÃO
 * Nos computadores modernos, o processador consegue operar diversas atividades simultaneamente através do uso de pipelines, estratégia eficiente que trouxe um aumento significativo no tempo de execução das tarefas pelo processador. Dessa maneira, ao invés de cada tarefa independe uma da outra ser feita em sequência, com o pipeline, elas podem ser realizadas simultaneamente. Contudo, para que isso seja possível, cada tarefa tem que ser independe entre si ou usando métodos que quebrem essa dependência, como usar múltiplas variáveis acumuladores.
 * Em relação aos níveis de otimização do compilador, tem-se:
 * - O0: nenhuma otimização, sendo o código exatamente o que foi escrito
 * - O1: otimizações básicas
 * - O2: otimizações intermediárias, reorganiza instruções
 * - O3: otimizações agressivas, pode vetorizar loops e muito mais
 */

 /**
  * EXPLICAÇÃO DA TAREFA
  * A finalidade da tarefa é comparar o tempo de processamento para:
  * - soma acumulativa em um vetor com dependência entre as iterações
  * - soma acumulativa em um vetor sem dependências e usando múltiplas variáveis
  */

  /**
   * COMO RESOLVER?
   * 1º) Definir um vetor grande ()
   * NOTA: Escolhi 10,000,000 para gerar um tempo de execução mensurável, e para forçar acesso na RAM e não ter caber facilmente no cache.
   * 2º) Implementar 3 laços: inicialização, soma com dependência e soma sem dependência
   * 3º) Analisar o tempo
   * 4º) Compilar com -O0, -O2 e -O3
   * 5º) Analisar as diferenças
   */


#include <stdio.h> 
#include <stdlib.h> 
#include <time.h>  

int main() {
    int tamanho = 10000000;
    double *vetor;
    vetor = (double *) malloc( (tamanho)*sizeof(double) );

    if (vetor==NULL) {
        printf("Erro: Faltou memória");
        return 1;
    }

    // Inicialização e atribuição de valores no vetor
    for (int i=0; i<tamanho; i++) {
        vetor[i] = 2*i;
    }

    // Soma acumulativa com dependência
    double soma = 0;
    clock_t inicio_com_dependencia = clock();

    for (int i=0; i<tamanho; i++) {
        soma += vetor[i];
    }

    printf("Soma com dependencia: %f\n", soma);

    clock_t fim_com_dependencia = clock();
    double diferenca_com_dependencia = ((double)(fim_com_dependencia - inicio_com_dependencia) * 1000) / CLOCKS_PER_SEC;
    printf("Tempo do vetor com dependencia: %f ms\n", diferenca_com_dependencia);

    // Soma acumulativa sem dependencia
    // Quebrando das dependencias com 4 variaveis
    double soma1 = 0;
    double soma2 = 0;
    double soma3 = 0;
    double soma4 = 0;

    clock_t inicio_sem_dependencia = clock();
    for (int i=0; i<tamanho; i+=4) {
        soma1 += vetor[i]; 
        soma2 += vetor[i+1]; 
        soma3 += vetor[i+2]; 
        soma4 += vetor[i+3]; 
    }
    double soma_total = soma1 + soma2 + soma3 + soma4;
    printf("Soma sem dependencia: %f\n", soma_total);
    
    clock_t fim_sem_dependencia = clock();
    double diferenca_sem_dependencia = ((double)(fim_sem_dependencia - inicio_sem_dependencia) * 1000) / CLOCKS_PER_SEC;
    printf("Tempo do vetor sem dependencia: %f ms\n\n", diferenca_sem_dependencia);

    // Fazendo a comparacao
    double diferenca = diferenca_com_dependencia - diferenca_sem_dependencia;
    printf("Diferenca total: %f ms\n", diferenca);


    free(vetor);
    return 0;
}

/**
 * CONCLUSÃO
 * Em -O0, o laço 3 (sem dependencia) foi mais rápido que o laço 2 (com dependencia). Isso ocorre pois sem otimização, as dependências de dados causam stalls no pipeline, impedindo o processador de executar instruções em paralelo. Quando houve otimização, o laço 2 melhorou, indicando que o compilador conseguiu reorganizar as instruções. Os níveis de -O2 e -O3 são similares, sugerindo que o ganho de desempenho obtido pela vetorização e reorganização de instruções já havia sido alcançado em -O2. Portanto, dependência de dados prejudica o desempenho do processador, e o compilador consegue minimizar esse impacto.
 */