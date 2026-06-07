#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    int rank;
    int quantidade_processos;

    // Inicializa o MPI
    MPI_Init(&argc, &argv);

    // rank é o número de cada processo
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // quantidade total de processos que estão rodando
    MPI_Comm_size(MPI_COMM_WORLD, &quantidade_processos);

    // Valores padrão, caso a pessoa rode sem passar M e N no terminal
    int M = 1200; // quantidade de linhas da matriz
    int N = 1200; // quantidade de colunas da matriz e tamanho do vetor x

    // Tempo de referência é o tempo com 1 processo
    // passa ele pelo terminal depois para calcular speedup e eficiência
    double tempo_referencia = 0.0;

    // Se a pessoa passar M e N no terminal, usa esses valores
    if (argc >= 3) {
        M = atoi(argv[1]);
        N = atoi(argv[2]);
    }

    // Se a pessoa passar o tempo de referência, usa ele também
    if (argc >= 4) {
        tempo_referencia = atof(argv[3]);
    }

    // Conferindo se M e N são válidos
    if (M <= 0 || N <= 0) {
        if (rank == 0) {
            printf("Erro: M e N precisam ser maiores que zero.\n");
        }

        MPI_Finalize();
        return 1;
    }

    // Como estamos usando MPI_Scatter, M precisa dividir certinho pela quantidade de processos
    // EM=1200 e processos=4  cada processo pega 300 linhas
    if (M % quantidade_processos != 0) {
        if (rank == 0) {
            printf("Erro: M precisa ser divisivel pela quantidade de processos.\n");
            printf("M = %d, processos = %d\n", M, quantidade_processos);
            printf("Tente usar M = 1200, 2400 ou 4800.\n");
        }

        MPI_Finalize();
        return 1;
    }

    int linhas_por_processo = M / quantidade_processos;
    int quantidade_elementos_local = linhas_por_processo * N;

    // Ponteiros principais
    double *matriz_A = NULL; // matriz completa, só existe no processo 0
    double *vetor_x = NULL; // vetor x, vai existir em todos os processos
    double *vetor_y = NULL; // vetor y completo, só existe no processo 0
    double *matriz_local = NULL; // pedaço da matriz que cada processo recebe
    double *vetor_y_local = NULL; // pedaço do resultado calculado por cada processo

    // Todos os processos precisam do vetor x
    vetor_x = (double *) malloc(N * sizeof(double));

    // Cada processo recebe só algumas linhas da matriz
    matriz_local = (double *) malloc(quantidade_elementos_local * sizeof(double));

    // Cada processo calcula só algumas posições do vetor y
    vetor_y_local = (double *) malloc(linhas_por_processo * sizeof(double));

    // Só o processo 0 guarda a matriz completa e o vetor y completo
    if (rank == 0) {
        matriz_A = (double *) malloc(M * N * sizeof(double));
        vetor_y = (double *) malloc(M * sizeof(double));
    }

    // Verificação simples de memória
    int erro_memoria = 0;

    if (vetor_x == NULL || matriz_local == NULL || vetor_y_local == NULL) {
        erro_memoria = 1;
    }

    if (rank == 0 && (matriz_A == NULL || vetor_y == NULL)) {
        erro_memoria = 1;
    }

    // Se qualquer processo teve erro de memória, todos ficam sabendo
    int erro_global = 0;
    MPI_Allreduce(&erro_memoria, &erro_global, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

    if (erro_global == 1) {
        if (rank == 0) {
            printf("Erro: faltou memoria para alocar os vetores/matriz.\n");
        }

        free(vetor_x);
        free(matriz_local);
        free(vetor_y_local);

        if (rank == 0) {
            free(matriz_A);
            free(vetor_y);
        }

        MPI_Finalize();
        return 1;
    }

    // O processo 0 inicializa a matriz A e o vetor x
    if (rank == 0) {
        // Preenchendo a matriz com valores simples
        // Usei uma continha simples só pra não deixar tudo igual
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                matriz_A[i * N + j] = (double) ((i + j) % 10 + 1);
            }
        }

        // Preenchendo o vetor x
        // tudo 1.0 pra facilitar a conferência do resultado
        for (int j = 0; j < N; j++) {
            vetor_x[j] = 1.0;
        }
    }

    // Sincroniza todo mundo antes de começar a medir o tempo
    MPI_Barrier(MPI_COMM_WORLD);

    double inicio_total = MPI_Wtime();

    // Divide a matriz A por linhas entre os processos
    MPI_Scatter(matriz_A,quantidade_elementos_local, MPI_DOUBLE,matriz_local,quantidade_elementos_local,MPI_DOUBLE,
0, MPI_COMM_WORLD);

    // Envia o vetor x inteiro para todos os processos
    MPI_Bcast(vetor_x,N,MPI_DOUBLE,0,MPI_COMM_WORLD);

    // Medindo só o tempo de cálculo local também
    double inicio_calculo = MPI_Wtime();

    // Cada processo calcula as linhas que recebeu
    for (int i = 0; i < linhas_por_processo; i++) {
        double soma = 0.0;

        for (int j = 0; j < N; j++) {
            soma += matriz_local[i * N + j] * vetor_x[j];
        }

        vetor_y_local[i] = soma;
    }

    double fim_calculo = MPI_Wtime();

    // Junta todos os pedaços do vetor y no processo 0
    MPI_Gather(vetor_y_local, linhas_por_processo,MPI_DOUBLE,vetor_y,  linhas_por_processo, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    double fim_total = MPI_Wtime();

    double tempo_total_local = fim_total - inicio_total;
    double tempo_calculo_local = fim_calculo - inicio_calculo;

    // Como cada processo pode terminar em tempos um pouco diferentes,
    // pegamos o maior tempo, pois o programa só termina quando todos terminam
    double tempo_total = 0.0;
    double tempo_calculo = 0.0;

    MPI_Reduce(&tempo_total_local, &tempo_total, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&tempo_calculo_local, &tempo_calculo, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    // Processo 0 imprime os resultados finais
    if (rank == 0) {
        double checksum = 0.0;

        // Soma o vetor y só pra garantir que o resultado foi realmente usado
        // e também pra comparar se dá o mesmo valor com vários processos
        for (int i = 0; i < M; i++) {
            checksum += vetor_y[i];
        }

        printf("\nRESULTADO\n");
        printf("Matriz A: %d x %d\n", M, N);
        printf("Processos: %d\n", quantidade_processos);
        printf("Linhas por processo: %d\n", linhas_por_processo);
        printf("Tempo total: %f segundos\n", tempo_total);
        printf("Tempo apenas do calculo local: %f segundos\n", tempo_calculo);
        printf("Checksum do vetor y: %.2f\n", checksum);

        // Se a pessoa passou o tempo de referência, calcula speedup e eficiência
        if (tempo_referencia > 0.0) {
            double speedup = tempo_referencia / tempo_total;
            double eficiencia = (speedup / quantidade_processos) * 100.0;

            printf("Tempo de referencia com 1 processo: %f segundos\n", tempo_referencia);
            printf("Speedup: %.2fx\n", speedup);
            printf("Eficiencia: %.2f%%\n", eficiencia);
        } else {
            printf("Tempo de referencia nao informado.\n");
            printf("Se este teste foi com 1 processo, use o Tempo total como referencia nos proximos testes\n");
        }
    }

    // free
    free(vetor_x);
    free(matriz_local);
    free(vetor_y_local);

    if (rank == 0) {
        free(matriz_A);
        free(vetor_y);
    }

    // Finaliza o MPI
    MPI_Finalize();

    return 0;
}