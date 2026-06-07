#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

// Tamanho da grade NxN -> o espaço fica quebrado em pontinhos (diferenças finitas)
#define N 1000

// Quantos passos no tempo a simulação vai dar (quantas vezes o fluido anda no tempo)
#define STEPS 600

// Parâmetros físicos da simulação
#define NU 0.2 // viscosidade do fluido 
#define DX 1.0 // distância entre dois pontos vizinhos da grade
#define DT 1.0 // tamanho do passo de tempo

// ALFA é o número de difusão: nu * dt / dx^2
// pra simulação ser estável em 2D, ALFA tem que ser <= 0.25, então 0.2 tá ok
#define ALFA (NU * DT / (DX * DX))

// Função pra reservar a grade na memória (vetor 1D de tamanho N*N usado como se fosse 2D)
double *aloca_grade() {
    double *g = (double *) malloc((N * N) * sizeof(double));
    if (g == NULL) {
        printf("Erro: Faltou memória!\n");
        exit(1);
    }
    return g;
}

// Deixa o fluido todo parado (velocidade = 0 em todo lugar)
void inicializa_repouso(double *u) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            u[i * N + j] = 0.0;
        }
    }
}

// Deixa o fluido parado mas coloca uma perturbação (um soco de velocidade) no meio
void inicializa_perturbacao(double *u) {
    inicializa_repouso(u); // primeiro zera tudo
    // coloca um valor alto no centro da grade pra ver se ele se espalha suave
    int meio = N / 2;
    u[meio * N + meio] = 100.0;
}

// Um passo no tempo sem paralelizar (versão sequencial)
// usa o vizinho de cima, de baixo, da esquerda e da direita pra calcular o novo valor
void passo_sequencial(double *u, double *u_novo) {
    for (int i = 1; i < N - 1; i++) {
        for (int j = 1; j < N - 1; j++) {
            u_novo[i * N + j] = u[i * N + j] + ALFA * (u[(i + 1) * N + j] + u[(i - 1) * N + j] + u[i * N + (j + 1)] + u[i * N + (j - 1)] - 4.0 * u[i * N + j]);
        }
    }
}

// VALIDAÇÃO DA FÍSICA (tudo sequencial aqui)
void validacao() {
    double *u = aloca_grade();
    double *u_novo = aloca_grade();

    // TESTE 1: fluido parado tem que continuar parado
    inicializa_repouso(u);
    for (int t = 0; t < STEPS; t++) {
        passo_sequencial(u, u_novo);
        double *tmp = u; u = u_novo; u_novo = tmp; // troca os ponteiros (double buffer)
    }
    // procura o maior valor pra ver se continuou tudo zero
    double maior = 0.0;
    for (int i = 0; i < N * N; i++) {
        if (fabs(u[i]) > maior) maior = fabs(u[i]);
    }
    printf("VALIDACAO\n");
    printf("Teste fluido parado -> maior velocidade no fim: %f (esperado)\n\n", maior);

    // TESTE 2: coloca uma perturbação no meio e vê se ela se difunde suave
    inicializa_perturbacao(u);
    int meio = N / 2;
    printf("Teste perturbacao (valor no centro ao longo do tempo):\n");
    for (int t = 0; t <= STEPS; t++) {
        // imprime de vez em quando pra acompanhar o espalhamento
        if (t % 150 == 0) {
            printf("  passo %4d -> centro: %8.4f | vizinho a 5 casas: %8.4f\n", t, u[meio * N + meio], u[meio * N + (meio + 5)]);
        }
        passo_sequencial(u, u_novo);
        double *tmp = u; u = u_novo; u_novo = tmp;
    }
    printf("\n");

    free(u);
    free(u_novo);
}

// VERSÕES PRA MEDIR TEMPO 

// Sequencial (sem OpenMP) -> serve de referência pro speedup
double simular_sequencial() {
    double *u = aloca_grade();
    double *u_novo = aloca_grade();
    inicializa_perturbacao(u);

    double inicio = omp_get_wtime();
    for (int t = 0; t < STEPS; t++) {
        passo_sequencial(u, u_novo);
        double *tmp = u; u = u_novo; u_novo = tmp;
    }
    double fim = omp_get_wtime();

    free(u);
    free(u_novo);
    return fim - inicio;
}

// Paralela básica: só joga o #pragma omp parallel for no laço de fora
double simular_parallel_for() {
    double *u = aloca_grade();
    double *u_novo = aloca_grade();
    inicializa_perturbacao(u);

    double inicio = omp_get_wtime();
    for (int t = 0; t < STEPS; t++) {
        #pragma omp parallel for
        for (int i = 1; i < N - 1; i++) {
            for (int j = 1; j < N - 1; j++) {
                u_novo[i * N + j] = u[i * N + j] + ALFA * (u[(i + 1) * N + j] + u[(i - 1) * N + j] + u[i * N + (j + 1)] + u[i * N + (j - 1)] - 4.0 * u[i * N + j]);
            }
        }
        double *tmp = u; u = u_novo; u_novo = tmp;
    }
    double fim = omp_get_wtime();

    free(u);
    free(u_novo);
    return fim - inicio;
}

// schedule(static): divide as linhas em blocos iguais, uma vez só, no começo
double simular_static() {
    double *u = aloca_grade();
    double *u_novo = aloca_grade();
    inicializa_perturbacao(u);

    double inicio = omp_get_wtime();
    for (int t = 0; t < STEPS; t++) {
        #pragma omp parallel for schedule(static)
        for (int i = 1; i < N - 1; i++) {
            for (int j = 1; j < N - 1; j++) {
                u_novo[i * N + j] = u[i * N + j] + ALFA * (u[(i + 1) * N + j] + u[(i - 1) * N + j] + u[i * N + (j + 1)] + u[i * N + (j - 1)] - 4.0 * u[i * N + j]);
            }
        }
        double *tmp = u; u = u_novo; u_novo = tmp;
    }
    double fim = omp_get_wtime();

    free(u);
    free(u_novo);
    return fim - inicio;
}

// schedule(dynamic): cada thread pega um pedacinho, quando termina pega outro
double simular_dynamic() {
    double *u = aloca_grade();
    double *u_novo = aloca_grade();
    inicializa_perturbacao(u);

    double inicio = omp_get_wtime();
    for (int t = 0; t < STEPS; t++) {
        #pragma omp parallel for schedule(dynamic)
        for (int i = 1; i < N - 1; i++) {
            for (int j = 1; j < N - 1; j++) {
                u_novo[i * N + j] = u[i * N + j] + ALFA * (u[(i + 1) * N + j] + u[(i - 1) * N + j] +u[i * N + (j + 1)] + u[i * N + (j - 1)] -4.0 * u[i * N + j]);
            }
        }
        double *tmp = u; u = u_novo; u_novo = tmp;
    }
    double fim = omp_get_wtime();

    free(u);
    free(u_novo);
    return fim - inicio;
}

// schedule(guided): começa com pedaços grandes e vai diminuindo
double simular_guided() {
    double *u = aloca_grade();
    double *u_novo = aloca_grade();
    inicializa_perturbacao(u);

    double inicio = omp_get_wtime();
    for (int t = 0; t < STEPS; t++) {
        #pragma omp parallel for schedule(guided)
        for (int i = 1; i < N - 1; i++) {
            for (int j = 1; j < N - 1; j++) {
                u_novo[i * N + j] = u[i * N + j] + ALFA * (
                    u[(i + 1) * N + j] + u[(i - 1) * N + j] +u[i * N + (j + 1)] + u[i * N + (j - 1)] -4.0 * u[i * N + j]);
            }
        }
        double *tmp = u; u = u_novo; u_novo = tmp;
    }
    double fim = omp_get_wtime();

    free(u);
    free(u_novo);
    return fim - inicio;
}

// collapse(2): junta os dois laços (i e j) num só, dando mais trabalho pra dividir
double simular_collapse() {
    double *u = aloca_grade();
    double *u_novo = aloca_grade();
    inicializa_perturbacao(u);

    double inicio = omp_get_wtime();
    for (int t = 0; t < STEPS; t++) {
        #pragma omp parallel for schedule(static) collapse(2)
        for (int i = 1; i < N - 1; i++) {
            for (int j = 1; j < N - 1; j++) {
                u_novo[i * N + j] = u[i * N + j] + ALFA * (u[(i + 1) * N + j] + u[(i - 1) * N + j] +u[i * N + (j + 1)] + u[i * N + (j - 1)] -4.0 * u[i * N + j]);
            }
        }
        double *tmp = u; u = u_novo; u_novo = tmp;
    }
    double fim = omp_get_wtime();

    free(u);
    free(u_novo);
    return fim - inicio;
}

int main() {
    printf("Grade: %dx%d | Passos no tempo: %d | Threads: %d | ALFA: %.3f\n\n",N, N, STEPS, omp_get_max_threads(), ALFA);

    // PARTE 1: validar se a física tá certa
    validacao();

    // PARTE 2: medir tempo das versões e calcular speedup
    printf("DESEMPENHO\n");
    double t_seq = simular_sequencial();
    printf("Sequencial -> %.4f s\n", t_seq);

    double t_par = simular_parallel_for();
    printf("parallel for -> %.4f s | speedup: %.2fx\n", t_par, t_seq / t_par);

    double t_static = simular_static();
    printf("schedule(static) -> %.4f s | speedup: %.2fx\n", t_static, t_seq / t_static);

    double t_dynamic = simular_dynamic();
    printf("schedule(dynamic) -> %.4f s | speedup: %.2fx\n", t_dynamic, t_seq / t_dynamic);

    double t_guided = simular_guided();
    printf("schedule(guided) -> %.4f s | speedup: %.2fx\n", t_guided, t_seq / t_guided);

    double t_collapse = simular_collapse();
    printf("static + collapse(2) -> %.4f s | speedup: %.2fx\n", t_collapse, t_seq / t_collapse);

    return 0;
}