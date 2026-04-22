/**
 * TAREFA 3 - ENUNCIADO
 * Implemente dois programas paralelos em C com OpenMP: um limitado por memória, com somas simples em vetores, e outro limitado por CPU, com cálculos matemáticos intensivos. Paralelize com `#pragma omp parallel for ` e avalie o desempenho. 

Analise quando o desempenho melhora, estabiliza ou piora, e reflita sobre como o multithreading de hardware pode ajudar em programas memory-bound, mas atrapalhar em programas compute-bound pela competição por recursos.

Explique quais são as melhores métricas para avaliar cada caso e como cada uma pode representar os diferentes aspectos do problema.
 */

/**
 * EXPLICAÇÃO
 * Ao longo do tempo, a capacidade de processamento do computador necessitou cada vez mais de velocidade para processar os problemas complexos demandados com precisão. Dessa maneira, a solução até então foi o aumento de clock dos processadores, que assim aumentava significativamente a velocidade de processamento. Contudo, isso chegou a um limite físico, como a geração excessiva de calor e o consumo alto de energia. Assim, fez-se a necessidade em pensar novas soluções para essa problemática, como a criação de processadores multicore, que possibilitou a programação paralela e melhora na programação concorrente.
 * - Programação concorrente: "múltiplas tarefas progridem no mesmo período de tempo, mas não necessariamente ao mesmo instante", ou seja, o sistema operacional faz um escalonamento de processos através das políticas de escalonamento (como prioridade, Round Robin, etc.), necessitando salvar o estado atual na PCB (Process Control Block), carregar o estado de outra tarefa, executar ela por um tempo, e depois repetir todo o processo de troca de contexto. Todavia, isso tem um custo, salvar e restaurar leva tempo.
 * NOTA: precisa de no mínimo um núcleo para existir
 * - Programação paralela: "múltiplas tarefas executam literalmente ao mesmo instante, em unidades de processamente diferentes", ou seja, cada núcleo tem sua própria unidade de processamento e registradores, trabalhando independentemente, e essas tarefas são executadas simultaneamente.
 * NOTA: precida de múltiplas unidades de processamento.
 * No entanto, mesmo com essas inovações de hardware e processamento, os computadores ainda sofrem com o gargalo de rapidez na leitura da memória vs velocidade de processamento. Assim, segue os conceitos abaixo:
 * - Memory bound: quando um programa passa por memory bound, a CPU passa a maior parte do tempo ociosa esperando dados chegarem da memória, não calculando.
 * - Compute bound: a CPU está sempre ocupada calculando pois o processamento em si é pesado, mesmo que a memória consiga entregar dados rapidamente.
 * De maneira análoga, é como uma corrente com um elo mais fraco e outro mais forte. Quando a memória RAM é rápida, entrega muitos dados a CPU que não consegue suprir essa alta demanda (compute bound); quando a CPU é veloz, a memória não consegue suprir essa alta velocidade (memory bound). Além disso, deve-se considerar fatores físicos como o limite de velocidade de transferência, também que quanto mais velocidade, mais calor, o que necessita de um mecanismo de resfriamento otimizado, e mesmo o SSD NVMe rápido ainda é em ordens de magnitude mais lento que CPU.
 * A largura do cano, ou chamado de Bandwidth, é a quantidade de dados que pode ser transferida entre RAM e CPU por segundo.
 * Banswidth = quantidade de dados / tempo
 * Unidade de medida: GB/s
 * Em relação ao bandwidth e quantidade de threads, um programa memory bound com uma thread, pode sobrar espaço disponível no bandwidth, já com múltiplas threads, pode saturar, criando filas sem real ganho. 
 * Tempo de transferência = tamanho dos dados / bandwidth
 * 
 * # OpenMP
 * É uma API que permite escrever código paralelo em C de forma simples, ao invés de ter que manipular o loop a fim de paralelizar. Para compilar, tem que usar a flag -fopenmp
 * `gcc programa.c -o programa -fopenmp`
 * Por padrão, as variáveis elas podem ser shared ou private.
 * - Shared: as variáveis são declaradas fora do bloco paralelo, todas as threads leem e escrevem na mesma posição de memória;
 * - Private: cda thread tem sua própria cópia da variável, logo modificações de uma thread não afetam as outras.
 * 
 * # Speedup e eficiência
 * O speedup é uma forma de analisar se o uso de paralelismo trouxe mais rapidez de processamento ou não. Assim, pode ser calculado pela relação:
 * speedup = Tempo com 1 thread / tempo com N threads
 * Speedup > 1, paralelismo ajudou
 * Speedup = 1, paralelismo não ajudou
 * Speedup < 1, paralelismo piorou (overhead)
 * Speedup = N, speedup linear, o que é ideal, mas raramente acontece
 * Speedup > N, superlinear
 * A partir dessa relação, é possível calcular a Eficiência:
 * eficiência = speedup / Nº de threads = T(1) / N x T(N)
 * Eficiência = 100%, perfeito e sem despedício (impossível na prática)
 * Eficiência > 70%, bom aproveitamento
 * Eficiência > 50%, aceitável
 * Eficiência < 50, ruim (overhead dominando)
 * 
 * # Lei de Amdahl
 * A lei de Amdahl explica que em infinitas threads, o programa não será infinitamente mais rápido, já que a parte serial do programa (inicializção, leitura de arquivos, print final, etc.) não pode ser paralelizada, isto é, não pode ser acelerada independe de quantas threads sejam usadas.
 * No memory bound, essa lei aparece como a parte serial sendo igual ao tempo esperando memória, porque não diminui com mais threads. No compute bound, o serial é a inicialização ou sincronização.
 */

 /**
  * EXPLICAÇÃO DA TAREFA
  * 
  */

  /**
   * COMO RESOLVER?
   *    1º → Escreva o Programa 1 (memory-bound)
        2º → Teste com 1 thread (garantir que está correto)
        3º → Varie as threads e colete os tempos
        4º → Calcule as métricas do Programa 1

        5º → Escreva o Programa 2 (compute-bound)
        6º → Teste com 1 thread (garantir que está correto)
        7º → Varie as threads e colete os tempos
        8º → Calcule as métricas do Programa 2

        9º  → Monte a análise comparativa
        10º → Aplique a Lei de Amdahl
        11º → Escreva as conclusões
   */

#include <stdio.h> 
#include <stdlib.h> 
#include <omp.h> // OpenMP
#include <time.h> 

#define N 100000000 // pra alcançar a memória RAM e testar o memory bound
#define DELTA_TEMPO_P1_REF 0.253

int main() {
    // Criar os vetores
    double *vetor_soma;
    double *vetor_x;
    double *vetor_y;

    vetor_soma = (double *) malloc( (N)*sizeof(double) );
    vetor_x = (double *) malloc( (N)*sizeof(double) );
    vetor_y = (double *) malloc( (N)*sizeof(double) );

    if (vetor_soma == NULL || vetor_x == NULL || vetor_y == NULL) {
        printf("Erro: Faltou memória!\n");
        return 1;
    } 

    // Programa 1: soma dos vetores

    // Atribuição de valores aos vetores
    for (int i=0; i<N; i++) {
        vetor_x[i] = (double) i;
        vetor_y[i] = (double) i;
    }

    // iniciar contagem de tempo
    double tempo_inicio_p1 = omp_get_wtime();

    // Soma dos vetores x e y
    #pragma omp parallel for // paralelizar
    for (int j=0; j<N; j++) {
        vetor_soma[j] = vetor_x[j] + vetor_y[j];
    }

    // Tempo final
    double tempo_fim_p1 = omp_get_wtime();
    double delta_tempo_p1 = tempo_fim_p1 - tempo_inicio_p1;
    printf("Threads: %d | Tempo: %fseg\n", omp_get_max_threads(), delta_tempo_p1);

    // Cálculo do speedup e eficiecia
    double speedup_p1 = DELTA_TEMPO_P1_REF / delta_tempo_p1;
    double eficiencia_p1 = (speedup_p1 / omp_get_max_threads())*100;
    printf("Speedup: %.2fx | Eficiencia: %.2f%%\n", speedup_p1, eficiencia_p1);

    free(vetor_soma);
    free(vetor_x);
    free(vetor_y);

    return 0;
}

/**
 * Análise dos resultados
 * 
 */