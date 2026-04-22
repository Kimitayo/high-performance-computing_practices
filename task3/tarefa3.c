/**
 * TAREFA 3 - ENUNCIADO
 * Implemente um programa em C que calcule uma aproximação de π por meio de uma série matemática, variando o número de iterações e medindo o tempo de execução. Compare os valores obtidos com o valor real de π e analise como a acurácia melhora com mais processamento. Reflita sobre como esse comportamento se repete em aplicações reais que exigem resultados cada vez mais precisos, como simulações físicas e inteligência artificial.
 */

/**
 * EXPLICAÇÃO
 * Quando é solicitado na linguagem C o valor valor de pi, o computador está apenas fazendo uma leitura de um valor salvo, sendo que finito, já que toda memória física tem um limite. Então, para conseguir valores mais precisos para pi, tendendo ao infinito, é feito um cálculo utilizando séries matemáticas como a de Leibniz pi = 4 * (1 - 1/3 + 1/5 - 1/7 + 1/9 - ...). Quanto mais tendendo ao infinito, isto é, quanto mais iterações sendo feitas, mais preciso é esse resultado de pi, contudo, mais tempo de processamento é necessário nesse processo. Situação análoga acontece ao tentar prever situações complexas atualmente, e por isso a necessidade de computadores com processadores mais otimizados e eficientes que consigam chegar a resultados mais precisos em menor tempo. Para essa tarefa, estarei implementando série de Nilakantha, por convergir muito mais rápido que a de Leibniz e ainda ser relativamente simples de programar em C.
 */

 /**
  * EXPLICAÇÃO DA TAREFA
  * Calcular o valor de pi de forma bruta
  * cada iteração é mais um termo sendo subtraído ou somado na série
  * calcular o tempo de cada iteração (10, 1000, 1,000,000, 1,000,000,000)
  * usr M_PI para comparar com o que o programa calculou e ver o quanto errou.
  */

  /**
   * COMO RESOLVER?
   * 1º) Montar a série matemática.
   * NOTA: foi escolhida a série de Nilakantha
   * 2º) Montar o laço de repetição com o cálculo da série
   * 3º) Criar o cronômetro
   * 4º) Variar as iterações (10; 1000; 1,000,000; 1,000,000,000)
   * 5º) Comparar os resultados do tempo de cada iteração e do valor de pi real para o pi calculado nessa aplicação.
   */

#include <stdio.h> 
#include <stdlib.h> 
#include <time.h> 
#include <math.h>

int main() {
    long iteracao[] = {10, 1000, 1000000, 1000000000};
    int tam_iteracao = 4;

    // Loop geral do vetor iteracao
    for (int i=0; i<tam_iteracao; i++) { // i=0 - 3
        double tempo_inicial = clock();
        double calculo_pi = 0;

        // Montagem da série de Nilakantha
        for (long j=0; j<iteracao[i]; j++) { // j=0 - 10
            if (j == 0) {
                calculo_pi = 3;
            } else{
                if (j % 2 != 0) {
                    calculo_pi += (4.0 / (2 * (double)j * ((2 * (double)j) + 1) * ((2 * (double)j) + 2))); 
                } else {
                    calculo_pi -= (4.0 / (2 * (double)j * ((2 * (double)j) + 1) * ((2 * (double)j) + 2))); 
                }
            }
        }
        double tempo_final = clock();
        double tempo_gasto = (double)(tempo_final - tempo_inicial) * 1000 / CLOCKS_PER_SEC;

        printf("Iterações: %ld, Tempo gasto: %f ms, Valor de pi: %f, Erro: %f\n", iteracao[i], tempo_gasto, calculo_pi, calculo_pi - M_PI);
    }


    return 0;
}


/**
 * CONCLUSÃO
 * Os resultados demonstram que a acurácia melhora rapidamente nas primeiras iterações, mas tende a se estabilizar com o aumento do número de iterações. Com apenas 1000 iterações, o valor de pi já atingiu uma boa precisão para 6 casas decimais. Contudo, o tempo de processamento cresce proporcionalmente ao número de iterações, então enquanto 1000 iterações levaram apenas 0.012ms, 1 bilhão de iterações demandou aproximadamente 5,6 segundos, sem ganho perceptível de precisão. Portanto, é possível concluir que o custo computacional cresce proporcionalmente, mas o ganho em precisão diminui progressivamente, o que significa que a partir de um certo ponto, mais processamento gera retorno cada vez menor em qualidade de resultado.
 */