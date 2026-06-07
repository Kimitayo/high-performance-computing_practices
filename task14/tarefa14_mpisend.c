#include <stdio.h>
#include <stdlib.h>
#include <mpi.h> // biblioteca do MPI

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv); // liga o MPI, sempre tem que vir primeiro

    int rank; // pra saber quem eu sou: processo 0 ou processo 1
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // tamanhos de mensagem em bytes, do pequeninho ate 1MB
    int tamanhos[] = {8, 64, 512, 4096, 32768, 262144, 1048576};
    int qtd = 7;
    int repeticoes = 1000; // quantas idas e voltas pra cada tamanho (pra dar pra medir direito)

    if (rank == 0) {
        printf("Versao MPI_Send\n");
    }

    // loop que passa por cada tamanho de mensagem
    for (int t = 0; t < qtd; t++) {
        int tam = tamanhos[t];

        // cria a mensagem com tam bytes
        char *mensagem = (char*) malloc(tam);
        for (int i = 0; i < tam; i++) {
            mensagem[i] = 'a'; // enche de qualquer coisa, o conteudo nao importa
        }

        MPI_Barrier(MPI_COMM_WORLD); // espera os 2 processos chegarem aqui antes de medir
        double inicio = MPI_Wtime();  // cronometro do MPI ligando

        // aqui acontece o ping-pong varias vezes seguidas
        for (int r = 0; r < repeticoes; r++) {
            if (rank == 0) {
                // processo 0 manda e depois espera a resposta voltar
                MPI_Send(mensagem, tam, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
                MPI_Recv(mensagem, tam, MPI_CHAR, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else {
                // processo 1 espera chegar e ja devolve a mesma mensagem
                MPI_Recv(mensagem, tam, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(mensagem, tam, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            }
        }

        double fim = MPI_Wtime(); // cronometro desligando
        double tempo_total = fim - inicio;

        // so o processo 0 imprime, pra nao sair tudo duplicado na tela
        if (rank == 0) {
            // tempo de 1 ida e volta, em microseg
            double tempo_medio_us = (tempo_total / repeticoes) * 1000000.0;
            // largura de banda em MB/s -> vai e volta = 2x o tamanho
            double banda = (2.0 * tam * repeticoes) / tempo_total / (1024 * 1024);
            printf("Tamanho: %8d bytes | Tempo medio por troca: %10.3f us | Banda: %8.2f MB/s\n",
                   tam, tempo_medio_us, banda);
        }

        free(mensagem);
    }

    MPI_Finalize(); // desliga o MPI, sempre por ultimo
    return 0;
}