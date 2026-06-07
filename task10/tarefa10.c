#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <time.h>

#define N 100000000 // muitos pontos pra estressar a sincronizacao

// gerador de aleatorios proprio (igual ao da tarefa 6) -> usado no lugar do rand_r, já que é win
int meu_rand(unsigned int *seed) {
    *seed = *seed * 1103515245 + 12345;
    return (*seed >> 16) & 0x7fff; // retorna um numero de 0 ate 32767
}

// VERSAO 1: contador compartilhado + critical (essa eh a da tarefa 8)
void versao_compartilhado_critical() {
    double inicio = omp_get_wtime();
    long count = 0; // contador compartilhado entre todas as threads

    #pragma omp parallel
    {
        // cada thread tem sua propria seed -> evita race na geracao de aleatorios
        unsigned int seed = 12345 + omp_get_thread_num();

        #pragma omp for
        for (long i = 0; i < N; i++) {
            double x = (double)meu_rand(&seed) / 0x7fff; // ponto x entre 0 e 1
            double y = (double)meu_rand(&seed) / 0x7fff; // ponto y entre 0 e 1
            if (x*x + y*y <= 1.0) { 
                #pragma omp critical // so uma thread por vez pode mexer no count
                {
                    count++;
                }
            }
        }
    }

    double pi = (4.0 * count) / N;
    double fim = omp_get_wtime();
    printf("1) Compartilhado + critical -> pi: %.6f | Tempo: %fs\n", pi, fim - inicio);
}

// VERSAO 2: contador compartilhado + atomic (trocando critical por atomic)
void versao_compartilhado_atomic() {
    double inicio = omp_get_wtime();
    long count = 0;

    #pragma omp parallel
    {
        unsigned int seed = 12345 + omp_get_thread_num();

        #pragma omp for
        for (long i = 0; i < N; i++) {
            double x = (double)meu_rand(&seed) / 0x7fff;
            double y = (double)meu_rand(&seed) / 0x7fff;
            if (x*x + y*y <= 1.0) {
                #pragma omp atomic // operacao atomica direto no hardware, mais leve que o critical
                count++;
            }
        }
    }

    double pi = (4.0 * count) / N;
    double fim = omp_get_wtime();
    printf("2) Compartilhado + atomic   -> pi: %.6f | Tempo: %fs\n", pi, fim - inicio);
}

// VERSAO 3: contador privado + critical (sincroniza so uma vez no final)
void versao_privado_critical() {
    double inicio = omp_get_wtime();
    long count = 0;

    #pragma omp parallel
    {
        unsigned int seed = 12345 + omp_get_thread_num();
        long count_local = 0; // cada thread conta sozinha aqui, sem sincronizar nada

        #pragma omp for
        for (long i = 0; i < N; i++) {
            double x = (double)meu_rand(&seed) / 0x7fff;
            double y = (double)meu_rand(&seed) / 0x7fff;
            if (x*x + y*y <= 1.0) {
                count_local++; // sem sincronizacao nenhuma aqui dentro -> rapido
            }
        }

        // so sincroniza UMA vez por thread, no final
        #pragma omp critical
        {
            count += count_local;
        }
    }

    double pi = (4.0 * count) / N;
    double fim = omp_get_wtime();
    printf("3) Privado + critical       -> pi: %.6f | Tempo: %fs\n", pi, fim - inicio);
}

// VERSAO 4: contador privado + atomic (sincroniza so uma vez no final)
void versao_privado_atomic() {
    double inicio = omp_get_wtime();
    long count = 0;

    #pragma omp parallel
    {
        unsigned int seed = 12345 + omp_get_thread_num();
        long count_local = 0;

        #pragma omp for
        for (long i = 0; i < N; i++) {
            double x = (double)meu_rand(&seed) / 0x7fff;
            double y = (double)meu_rand(&seed) / 0x7fff;
            if (x*x + y*y <= 1.0) {
                count_local++;
            }
        }

        // junta o resultado local no compartilhado, uma vez por thread
        #pragma omp atomic
        count += count_local;
    }

    double pi = (4.0 * count) / N;
    double fim = omp_get_wtime();
    printf("4) Privado + atomic         -> pi: %.6f | Tempo: %fs\n", pi, fim - inicio);
}

// VERSAO 5: clausula reduction (deixa o OpenMP cuidar de tudo sozinho)
void versao_reduction() {
    double inicio = omp_get_wtime();
    long count = 0;

    #pragma omp parallel
    {
        unsigned int seed = 12345 + omp_get_thread_num();

        // reduction(+:count) -> cada thread ganha uma copia local e o OpenMP soma tudo no fim
        #pragma omp for reduction(+:count)
        for (long i = 0; i < N; i++) {
            double x = (double)meu_rand(&seed) / 0x7fff;
            double y = (double)meu_rand(&seed) / 0x7fff;
            if (x*x + y*y <= 1.0) {
                count++; // sem diretiva de sincronizacao nenhuma, o reduction resolve
            }
        }
    }

    double pi = (4.0 * count) / N;
    double fim = omp_get_wtime();
    printf("5) Reduction                -> pi: %.6f | Tempo: %fs\n", pi, fim - inicio);
}

int main() {
    printf("Estimativa de PI - Monte Carlo (N = %d pontos)\n", N);
    printf("Threads disponiveis: %d\n\n", omp_get_max_threads());

    versao_compartilhado_critical();
    versao_compartilhado_atomic();
    versao_privado_critical();
    versao_privado_atomic();
    versao_reduction();

    return 0;
}