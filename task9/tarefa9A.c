#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

#define N 1000000  // numero de insercoes

// Estrutura do no da lista encadeada
typedef struct No {
    int valor;
    struct No *proximo;
} No;

// Gerador de numeros aleatorios proprio (pro win)
// pois cada tarefa usa sua propria seed)
int meu_rand(unsigned int *seed) {
    *seed = *seed * 1103515245 + 12345;
    return (*seed >> 16) & 0x7fff;
}

// Inserir no inicio da lista (pra custo constante)
void inserir(No **cabeca, int valor) {
    No *novo = (No *) malloc(sizeof(No));
    novo->valor = valor;
    novo->proximo = *cabeca;
    *cabeca = novo;
}

// Contar quantos elementos a lista possui
int contar(No *cabeca) {
    int total = 0;
    while (cabeca != NULL) {
        total++;
        cabeca = cabeca->proximo;
    }
    return total;
}

// Liberar a memoria da lista
void liberar(No *cabeca) {
    while (cabeca != NULL) {
        No *temp = cabeca;
        cabeca = cabeca->proximo;
        free(temp);
    }
}

int main() {
    No *listaA = NULL;
    No *listaB = NULL;

    double inicio = omp_get_wtime();

    #pragma omp parallel
    {
        // Apenas uma thread cria as tarefas, as outras executam
        #pragma omp single
        {
            for (int i = 0; i < N; i++) {
                #pragma omp task firstprivate(i)
                {
                    // seed propria de cada tarefa
                    unsigned int seed = i + omp_get_thread_num() * 7919;
                    int escolha = meu_rand(&seed) % 2;  // 0 ou 1
                    int valor = meu_rand(&seed);

                    if (escolha == 0) {
                        // cadeado exclusivo da lista A
                        #pragma omp critical(listaA)
                        {
                            inserir(&listaA, valor);
                        }
                    } else {
                        // cadeado exclusivo da list B
                        #pragma omp critical(listaB)
                        {
                            inserir(&listaB, valor);
                        }
                    }
                }
            }
        }
    }

    double fim = omp_get_wtime();

    int qtdA = contar(listaA);
    int qtdB = contar(listaB);

    printf("Lista A: %d elementos\n", qtdA);
    printf("Lista B: %d elementos\n", qtdB);
    printf("Total inserido: %d (esperado: %d)\n", qtdA + qtdB, N);
    printf("Tempo: %f segundos\n", fim - inicio);

    liberar(listaA);
    liberar(listaB);

    return 0;
}