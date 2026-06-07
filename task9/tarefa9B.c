#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

#define N 1000000  // n de insercoes

typedef struct No {
    int valor;
    struct No *proximo;
} No;

// Gerador de numeros aleatorios proprio (pro win)
int meu_rand(unsigned int *seed) {
    *seed = *seed * 1103515245 + 12345;
    return (*seed >> 16) & 0x7fff;
}

void inserir(No **cabeca, int valor) {
    No *novo = (No *) malloc(sizeof(No));
    novo->valor = valor;
    novo->proximo = *cabeca;
    *cabeca = novo;
}

int contar(No *cabeca) {
    int total = 0;
    while (cabeca != NULL) {
        total++;
        cabeca = cabeca->proximo;
    }
    return total;
}

void liberar(No *cabeca) {
    while (cabeca != NULL) {
        No *temp = cabeca;
        cabeca = cabeca->proximo;
        free(temp);
    }
}

int main() {
    int num_listas = 8;  // deixar definido em 8

    // Vetor de listas (um ponteiro de cabeca por lista)
    No **listas = (No **) malloc(num_listas * sizeof(No *));
    // Vetor de locks: um cadeado por lista, criado em tempo de execucao
    omp_lock_t *locks = (omp_lock_t *) malloc(num_listas * sizeof(omp_lock_t));

    for (int i = 0; i < num_listas; i++) {
        listas[i] = NULL;
        omp_init_lock(&locks[i]);
    }

    double inicio = omp_get_wtime();

    #pragma omp parallel
    {
        #pragma omp single
        {
            for (int i = 0; i < N; i++) {
                #pragma omp task firstprivate(i)
                {
                    unsigned int seed = i + omp_get_thread_num() * 7919;
                    int escolha = meu_rand(&seed) % num_listas;
                    int valor = meu_rand(&seed);

                    // trava apenas o lock da lista escolhida
                    omp_set_lock(&locks[escolha]);
                    inserir(&listas[escolha], valor);
                    omp_unset_lock(&locks[escolha]);
                }
            }
        }
    }

    double fim = omp_get_wtime();

    int total = 0;
    for (int i = 0; i < num_listas; i++) {
        int qtd = contar(listas[i]);
        printf("Lista %d: %d elementos\n", i, qtd);
        total += qtd;
    }
    printf("Total inserido: %d (esperado: %d)\n", total, N);
    printf("Tempo: %f segundos\n", fim - inicio);

    // Destruir os locks e liberar a memoria
    for (int i = 0; i < num_listas; i++) {
        omp_destroy_lock(&locks[i]);
        liberar(listas[i]);
    }
    free(locks);
    free(listas);

    return 0;
}