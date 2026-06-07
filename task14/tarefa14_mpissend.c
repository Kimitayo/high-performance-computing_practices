#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int tamanhos[] = {8, 64, 512, 4096, 32768, 262144, 1048576};
    int qtd = 7;
    int repeticoes = 1000;

    if (rank == 0) {
        printf("Versao MPI_Ssend\n");
    }

    for (int t = 0; t < qtd; t++) {
        int tam = tamanhos[t];
        char *mensagem = (char*) malloc(tam);
        for (int i = 0; i < tam; i++) {
            mensagem[i] = 'a';
        }

        MPI_Barrier(MPI_COMM_WORLD);
        double inicio = MPI_Wtime();

        for (int r = 0; r < repeticoes; r++) {
            if (rank == 0) {
                // o Ssend so termina quando o processo 1 JA comecou a receber (sincrono)
                MPI_Ssend(mensagem, tam, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
                MPI_Recv(mensagem, tam, MPI_CHAR, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else {
                MPI_Recv(mensagem, tam, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Ssend(mensagem, tam, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            }
        }

        double fim = MPI_Wtime();
        double tempo_total = fim - inicio;

        if (rank == 0) {
            double tempo_medio_us = (tempo_total / repeticoes) * 1000000.0;
            double banda = (2.0 * tam * repeticoes) / tempo_total / (1024 * 1024);
            printf("Tamanho: %8d bytes | Tempo medio por troca: %10.3f us | Banda: %8.2f MB/s\n", tam, tempo_medio_us, banda);
        }

        free(mensagem);
    }

    MPI_Finalize();
    return 0;
}