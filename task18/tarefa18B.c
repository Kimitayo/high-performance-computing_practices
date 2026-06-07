#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    int rank;
    int quantidade_processos;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &quantidade_processos);

    int M = 1200;
    int N = 1200;
    double tempo_referencia = 0.0;

    if (argc >= 3) {
        M = atoi(argv[1]);
        N = atoi(argv[2]);
    }

    if (argc >= 4) {
        tempo_referencia = atof(argv[3]);
    }

    if (M <= 0 || N <= 0) {
        if (rank == 0) {
            printf("Erro: M e N precisam ser maiores que zero.\n");
        }

        MPI_Finalize();
        return 1;
    }

    if (N % quantidade_processos != 0) {
        if (rank == 0) {
            printf("Erro: N precisa ser divisivel pela quantidade de processos.\n");
            printf("N = %d, processos = %d\n", N, quantidade_processos);
        }

        MPI_Finalize();
        return 1;
    }

    int colunas_por_processo = N / quantidade_processos;

    double *matriz_A = NULL;
    double *matriz_local = NULL;

    double *vetor_x = NULL;
    double *vetor_x_local = NULL;

    double *vetor_y = NULL;
    double *vetor_y_parcial = NULL;

    matriz_local = (double *) malloc((size_t) M * colunas_por_processo * sizeof(double));
    vetor_x_local = (double *) malloc(colunas_por_processo * sizeof(double));
    vetor_y_parcial = (double *) malloc(M * sizeof(double));

    if (rank == 0) {
        matriz_A = (double *) malloc((size_t) M * N * sizeof(double));
        vetor_x = (double *) malloc(N * sizeof(double));
        vetor_y = (double *) malloc(M * sizeof(double));
    }

    int erro_memoria = 0;

    if (matriz_local == NULL || vetor_x_local == NULL || vetor_y_parcial == NULL) {
        erro_memoria = 1;
    }

    if (rank == 0 && (matriz_A == NULL || vetor_x == NULL || vetor_y == NULL)) {
        erro_memoria = 1;
    }

    int erro_global = 0;

    MPI_Allreduce(&erro_memoria, &erro_global, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

    if (erro_global == 1) {
        if (rank == 0) {
            printf("Erro: faltou memoria.\n");
        }

        free(matriz_local);
        free(vetor_x_local);
        free(vetor_y_parcial);

        if (rank == 0) {
            free(matriz_A);
            free(vetor_x);
            free(vetor_y);
        }

        MPI_Finalize();
        return 1;
    }

    if (rank == 0) {
        int i;
        int j;

        for (i = 0; i < M; i++) {
            for (j = 0; j < N; j++) {
                matriz_A[(size_t) i * N + j] = (double) ((i + j) % 10 + 1);
            }
        }

        for (j = 0; j < N; j++) {
            vetor_x[j] = 1.0;
        }
    }

    
       // cria o tipo vector normal-> representa um bloco de colunas, mas ainda tem o extent grande.
    
    MPI_Datatype tipo_colunas_temp;
    MPI_Datatype tipo_colunas_resized;

    MPI_Type_vector(M,colunas_por_processo, N,MPI_DOUBLE,&tipo_colunas_temp);

    /*
        ajusta o extend
        quando o MPI_Scatter for enviar para o processo 0, 1, 2...
        ele vai andar de colunas_por_processo em colunas_por_processo,
        e nao pelo extent gigante do MPI_Type_vector original
    */
    MPI_Type_create_resized(tipo_colunas_temp,0,colunas_por_processo * sizeof(double),&tipo_colunas_resized);

    MPI_Type_commit(&tipo_colunas_resized);

    MPI_Barrier(MPI_COMM_WORLD);

    double inicio_total = MPI_Wtime();

    MPI_Scatter(matriz_A,1,tipo_colunas_resized,matriz_local,M * colunas_por_processo,MPI_DOUBLE,0,MPI_COMM_WORLD);

    MPI_Scatter(vetor_x,colunas_por_processo,MPI_DOUBLE,vetor_x_local,colunas_por_processo,MPI_DOUBLE,0,MPI_COMM_WORLD);

    double inicio_calculo = MPI_Wtime();


        // Cada processo calcula uma contribuicao parcial para todos os elementos de y.-> Depois essas contribuicoes vao ser somadas com MPI_Reduce

    int i;
    int j;

    for (i = 0; i < M; i++) {
        double soma = 0.0;

        for (j = 0; j < colunas_por_processo; j++) {
            soma += matriz_local[(size_t) i * colunas_por_processo + j] * vetor_x_local[j];
        }

        vetor_y_parcial[i] = soma;
    }

    double fim_calculo = MPI_Wtime();

    MPI_Reduce(vetor_y_parcial,vetor_y,M,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);

    double fim_total = MPI_Wtime();

    double tempo_total_local = fim_total - inicio_total;
    double tempo_calculo_local = fim_calculo - inicio_calculo;

    double tempo_total = 0.0;
    double tempo_calculo = 0.0;

    MPI_Reduce(&tempo_total_local, &tempo_total, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&tempo_calculo_local, &tempo_calculo, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        double checksum = 0.0;

        for (i = 0; i < M; i++) {
            checksum += vetor_y[i];
        }

        printf("\nRESULTADO - MPI_TYPE_CREATE_RESIZED\n");
        printf("Matriz A: %d x %d\n", M, N);
        printf("Processos: %d\n", quantidade_processos);
        printf("Colunas por processo: %d\n", colunas_por_processo);
        printf("Tempo total: %f segundos\n", tempo_total);
        printf("Tempo apenas do calculo local: %f segundos\n", tempo_calculo);
        printf("Checksum do vetor y: %.2f\n", checksum);

        if (tempo_referencia > 0.0) {
            double speedup = tempo_referencia / tempo_total;
            double eficiencia = (speedup / quantidade_processos) * 100.0;

            printf("Tempo de referencia com 1 processo: %f segundos\n", tempo_referencia);
            printf("Speedup: %.2fx\n", speedup);
            printf("Eficiencia: %.2f%%\n", eficiencia);
        } else {
            printf("Tempo de referencia nao informado.\n");
        }
    }

    MPI_Type_free(&tipo_colunas_resized);
    MPI_Type_free(&tipo_colunas_temp);

    free(matriz_local);
    free(vetor_x_local);
    free(vetor_y_parcial);

    if (rank == 0) {
        free(matriz_A);
        free(vetor_x);
        free(vetor_y);
    }

    MPI_Finalize();

    return 0;
}