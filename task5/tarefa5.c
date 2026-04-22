/*
 TAREFA 3 - ENUNCIADO
 Implemente um programa em C que conte quantos números primos existem entre 2 e um valor máximo n. Depois, paralelize o laço principal usando a diretiva `#pragma omp parallel for` sem alterar a lógica original. compare o tempo de execução e os resultados das versões sequencial e paralela. observe possíveis diferenças no resultado e no desempenho, e reflita sobre os desafios iniciais da programação paralela, como correção e distribuição de carga.
 */

/*
  EXPLICAÇÃO
Nas análises anteriores, foi usada a biblioteca OpenMP a fim de pararelizar várias tarefas e executá-las simultaneamente usando múltiplas threads. A diretiva para fazer essa paralelização é `#pragma omp parallel for`, colocada antes de um laço for.
Contudo, em variáveis compartilhadas processadas em múltiplas threads, pode ocorrer do mesmo dado serem acessados e modificados simultaneamente por essas threads, e o resultado depender da ordem exata em que isso acontecer. Este problema é chamado de race condition. Para resolver essa vulnerabilidade, existem formas de proteger essa variável, e uma delas é o uso da cláusula reduction do OpenMP: `#pragma omp parallel for reduction()`. Dessa maneira, o loop será dividido entre as threads, em que cada um terá uma cópia local do processo; cada thread acumula sua parte e, no final, o OpenMP combina todas as cópias locais usando +.

- Compilação com OpenMP
A compilação com OpenMP utiliza uma flag especial "-fopenmp":
`gcc -fopenmp programa.c -o programa -lm`

- Medindo tempo de execução
Para fazer a diferenciação do tempo de execução sequencial vs paralelo, pode ser usada a função `omp_get_wtime()` oferecido pela OpenMP, que funcionará como um cronômetro.
 */

 /*
   EXPLICAÇÃO DA TAREFA
   
  */

  /*
    COMO RESOLVER?
    1º) Função eh_primo;
    2º) Versão sequencial
    3º) Versão paralela com OpenMP
    4º) Comparar resultados e tempos
   */

#include <stdio.h> 
#include <stdlib.h> 
#include <math.h>  
#include <omp.h>


// Função é primo sequencial
int eh_primo(int N) {
    for (int i = 2; i <= sqrt(N); i++) {
        if (N % i == 0) {
            return 0;  // Não é primo
        }
    }
    return 1;  // É primo
}

int main() {
    int n = 10000000; 

    // VERSÃO SEQUENCIAL
    int contador_seq = 0;
    double inicio_seq = omp_get_wtime();

    for (int i = 2; i <= n; i++) {
        if (eh_primo(i)) {
            contador_seq++;
        }
    }

    double fim_seq = omp_get_wtime();
    double tempo_seq = fim_seq - inicio_seq;

    printf("VERSAO SEQUENCIAL\n");
    printf("Primos encontrados: %d\n", contador_seq);
    printf("Tempo: %f segundos\n\n", tempo_seq);

    // VERSÃO PARALELA
    int contador_par = 0;
    double inicio_par = omp_get_wtime();

    #pragma omp parallel for reduction(+:contador_par)
    for (int i = 2; i <= n; i++) {
        if (eh_primo(i)) {
            contador_par++;
        }
    }

    double fim_par = omp_get_wtime();
    double tempo_par = fim_par - inicio_par;

    printf("VERSÃO PARALELA\n");
    printf("Primos encontrados: %d\n", contador_par);
    printf("Tempo: %f segundos\n\n", tempo_par);

    // COMPARAÇÃO
    printf("COMPARAÇAO\n");
    printf("Resultados iguais? %s\n", 
           contador_seq == contador_par ? "SIM" : "NÃO");
    printf("Speedup: %.2fx mais rapido\n", tempo_seq / tempo_par);

    return 0;
}