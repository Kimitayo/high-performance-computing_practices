#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define N_GLOBAL 1000000 // numero total de pontos da barra
#define PASSOS 1000 // quantos passos de tempo a simulacao roda
#define ALPHA  0.1 // velocidade da difusao (tem que ser <= 0.5)

// VERSAO 1: MPI_Send / MPI_Recv 
void versao_1() {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // numero desse processo
    MPI_Comm_size(MPI_COMM_WORLD, &size); // total de processos

    int local_n = N_GLOBAL / size; // pedaco da barra que esse processo cuida

    // descobrir os vizinhos
    int viz_esq = rank - 1;
    int viz_dir = rank + 1;
    if (rank == 0) viz_esq = MPI_PROC_NULL; // o primeiro nao tem vizinho a esquerda
    if (rank == size - 1) viz_dir = MPI_PROC_NULL; // o ultimo nao tem vizinho a direita

    // 2 posicoes a mais: uma celula fantasma em cada ponta
    double *atual = malloc((local_n + 2) * sizeof(double));
    double *novo  = malloc((local_n + 2) * sizeof(double));

    // comeca tudo frio (zero)
    for (int i = 0; i < local_n + 2; i++) { atual[i] = 0; novo[i] = 0; }
    if (rank == 0) atual[1] = 100; // ponta esquerda quente

    MPI_Barrier(MPI_COMM_WORLD); // todo mundo comeca junto
    double inicio = MPI_Wtime();

    for (int passo = 0; passo < PASSOS; passo++) {
        // troca as bordas com os vizinhos (mensagem pequena, nao trava)
        MPI_Send(&atual[local_n], 1, MPI_DOUBLE, viz_dir, 0, MPI_COMM_WORLD);
        MPI_Recv(&atual[0], 1, MPI_DOUBLE, viz_esq, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(&atual[1], 1, MPI_DOUBLE, viz_esq, 0, MPI_COMM_WORLD);
        MPI_Recv(&atual[local_n+1], 1, MPI_DOUBLE, viz_dir, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // calcula a temperatura nova de cada ponto
        for (int i = 1; i <= local_n; i++) {
            novo[i] = atual[i] + ALPHA * (atual[i-1] - 2*atual[i] + atual[i+1]);
        }

        // mantem as pontas da barra fixas
        if (rank == 0)  novo[1] = 100;
        if (rank == size - 1) novo[local_n] = 0;

        // copia o "novo" pro "atual" pro proximo passo
        for (int i = 1; i <= local_n; i++) {
            atual[i] = novo[i];
        }
    }

    double fim = MPI_Wtime();

    // soma todo o calor (so pra conferir se as 3 versoes batem)
    double soma_local = 0, soma_total = 0;
    for (int i = 1; i <= local_n; i++) soma_local += atual[i];
    MPI_Reduce(&soma_local, &soma_total, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0)
        printf("Versao 1 (Send/Recv) -> Tempo: %f s | Calor: %f\n", fim - inicio, soma_total);

    free(atual); free(novo);
}

// VERSAO 2: MPI_Isend / MPI_Irecv + MPI_Wait 
void versao_2() {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int local_n = N_GLOBAL / size;
    int viz_esq = rank - 1;
    int viz_dir = rank + 1;
    if (rank == 0) viz_esq = MPI_PROC_NULL;
    if (rank == size - 1) viz_dir = MPI_PROC_NULL;

    double *atual = malloc((local_n + 2) * sizeof(double));
    double *novo  = malloc((local_n + 2) * sizeof(double));
    for (int i = 0; i < local_n + 2; i++) { atual[i] = 0; novo[i] = 0; }
    if (rank == 0) atual[1] = 100;

    MPI_Barrier(MPI_COMM_WORLD);
    double inicio = MPI_Wtime();

    for (int passo = 0; passo < PASSOS; passo++) {
        MPI_Request reqs[4];
        // dispara as 4 comunicacoes (nao trava o programa)
        MPI_Irecv(&atual[0], 1, MPI_DOUBLE, viz_esq, 0, MPI_COMM_WORLD, &reqs[0]);
        MPI_Irecv(&atual[local_n+1], 1, MPI_DOUBLE, viz_dir, 0, MPI_COMM_WORLD, &reqs[1]);
        MPI_Isend(&atual[1], 1, MPI_DOUBLE, viz_esq, 0, MPI_COMM_WORLD, &reqs[2]);
        MPI_Isend(&atual[local_n], 1, MPI_DOUBLE, viz_dir, 0, MPI_COMM_WORLD, &reqs[3]);

        // espera as 4 terminarem antes de calcular
        MPI_Waitall(4, reqs, MPI_STATUSES_IGNORE);

        for (int i = 1; i <= local_n; i++) {
            novo[i] = atual[i] + ALPHA * (atual[i-1] - 2*atual[i] + atual[i+1]);
        }
        if (rank == 0) novo[1] = 100;
        if (rank == size - 1) novo[local_n] = 0;
        for (int i = 1; i <= local_n; i++) {
            atual[i] = novo[i];
        }
    }

    double fim = MPI_Wtime();
    double soma_local = 0, soma_total = 0;
    for (int i = 1; i <= local_n; i++) soma_local += atual[i];
    MPI_Reduce(&soma_local, &soma_total, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    if (rank == 0)
        printf("Versao 2 (Isend/Irecv+Wait) -> Tempo: %f s | Calor: %f\n", fim - inicio, soma_total);
    free(atual); free(novo);
}

// VERSAO 3: MPI_Test (calcula enquanto comunica)
void versao_3() {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int local_n = N_GLOBAL / size;
    int viz_esq = rank - 1;
    int viz_dir = rank + 1;
    if (rank == 0) viz_esq = MPI_PROC_NULL;
    if (rank == size - 1) viz_dir = MPI_PROC_NULL;

    double *atual = malloc((local_n + 2) * sizeof(double));
    double *novo  = malloc((local_n + 2) * sizeof(double));
    for (int i = 0; i < local_n + 2; i++) { atual[i] = 0; novo[i] = 0; }
    if (rank == 0) atual[1] = 100;

    MPI_Barrier(MPI_COMM_WORLD);
    double inicio = MPI_Wtime();

    for (int passo = 0; passo < PASSOS; passo++) {
        MPI_Request reqs[4];
        MPI_Irecv(&atual[0], 1, MPI_DOUBLE, viz_esq, 0, MPI_COMM_WORLD, &reqs[0]);
        MPI_Irecv(&atual[local_n+1], 1, MPI_DOUBLE, viz_dir, 0, MPI_COMM_WORLD, &reqs[1]);
        MPI_Isend(&atual[1], 1, MPI_DOUBLE, viz_esq, 0, MPI_COMM_WORLD, &reqs[2]);
        MPI_Isend(&atual[local_n], 1, MPI_DOUBLE, viz_dir, 0, MPI_COMM_WORLD, &reqs[3]);

        // enquanto a comunicacao acontece, ja calculo os pontos do MEIO
        // nao preciso esperar)
        for (int i = 2; i <= local_n - 1; i++) {
            novo[i] = atual[i] + ALPHA * (atual[i-1] - 2*atual[i] + atual[i+1]);
        }

        // fico perguntando se a comunicacao ja terminou
        int terminou = 0;
        while (!terminou) {
            MPI_Testall(4, reqs, &terminou, MPI_STATUSES_IGNORE);
        }

        // agora que as bordas chegaram, calculo os 2 pontos das pontas
        novo[1] = atual[1] + ALPHA * (atual[0] - 2*atual[1] + atual[2]);
        novo[local_n] = atual[local_n] + ALPHA * (atual[local_n-1] - 2*atual[local_n] + atual[local_n+1]);

        if (rank == 0) novo[1] = 100;
        if (rank == size - 1) novo[local_n] = 0;
        for (int i = 1; i <= local_n; i++) {
            atual[i] = novo[i];
        }
    }

    double fim = MPI_Wtime();
    double soma_local = 0, soma_total = 0;
    for (int i = 1; i <= local_n; i++) soma_local += atual[i];
    MPI_Reduce(&soma_local, &soma_total, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    if (rank == 0)
        printf("Versao 3 (Test/sobreposicao) -> Tempo: %f s | Calor: %f\n", fim - inicio, soma_total);
    free(atual); free(novo);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) printf("Rodando com %d processos\n", size);

    versao_1();
    versao_2();
    versao_3();

    MPI_Finalize();
    return 0;
}