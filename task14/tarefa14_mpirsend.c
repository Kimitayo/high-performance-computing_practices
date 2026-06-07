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
        printf("Versao MPI_Rsend\n");
    }

    for (int t = 0; t < qtd; t++) {
        int tam = tamanhos[t];
        // aqui preciso de 2 buffers: um pra mandar e outro pra receber
        // isso porque o recebimento fica "aberto" (Irecv) enquanto eu mando,
        // entao nao da pra usar o mesmo buffer pras duas coisas ao mesmo tempo.
        char *mensagem = (char*) malloc(tam);
        char *recebido = (char*) malloc(tam);
        for (int i = 0; i < tam; i++) {
            mensagem[i] = 'a';
        }

        MPI_Barrier(MPI_COMM_WORLD);
        double inicio = MPI_Wtime();

        for (int r = 0; r < repeticoes; r++) {
            // o Rsend SO funciona se o recebimento do outro lado JA estiver postado.
            // por isso  posta o recv antes (MPI_Irecv) e usa o barrier pra garantir
            // que os 2 lados ja postaram o recv, ai sim pode mandar com seguranca.
            MPI_Request pedido;
            if (rank == 0) {
                MPI_Irecv(recebido, tam, MPI_CHAR, 1, 0, MPI_COMM_WORLD, &pedido); // ja deixa o recv pronto
                MPI_Barrier(MPI_COMM_WORLD); // garante que o processo 1 tambem ja postou o dele
                MPI_Rsend(mensagem, tam, MPI_CHAR, 1, 0, MPI_COMM_WORLD); // agora pode mandar
                MPI_Wait(&pedido, MPI_STATUS_IGNORE); // espera a resposta chegar
            } else {
                MPI_Irecv(recebido, tam, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &pedido);
                MPI_Barrier(MPI_COMM_WORLD);
                MPI_Wait(&pedido, MPI_STATUS_IGNORE); // espera a mensagem do processo 0 chegar
                MPI_Rsend(recebido, tam, MPI_CHAR, 0, 0, MPI_COMM_WORLD); // devolve a mesma mensagem
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
        free(recebido);
    }

    MPI_Finalize();
    return 0;
}