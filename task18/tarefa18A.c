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
    double *matriz_envio = NULL;
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

    /*
        tipo derivado com MPI_Type_vector.

        Esse tipo representa um bloco de colunas
        count = M -> quantidade de linhas
        blocklength = colunas_por_processo -> quantas colunas seguidas em cada linha
        stride = N -> salto para chegar na proxima linha da matriz original

        O problema é que o extent natural desse tipo fica grande.
        nessa versao, o processo 0 cria um buffer auxiliar.
    */
    MPI_Datatype tipo_colunas;

    MPI_Type_vector(M,colunas_por_processo,N,MPI_DOUBLE,&tipo_colunas);

    MPI_Type_commit(&tipo_colunas);

    MPI_Aint limite_inferior;
    MPI_Aint extent_bytes;

    MPI_Type_get_extent(tipo_colunas, &limite_inferior, &extent_bytes);

    size_t extent_elementos = (size_t) (extent_bytes / sizeof(double));

    if (rank == 0) {
        matriz_envio = (double *) malloc(extent_elementos * quantidade_processos * sizeof(double));

        if (matriz_envio == NULL) {
            printf("Erro: faltou memoria para o buffer auxiliar da versao vector.\n");

            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    double inicio_total = MPI_Wtime();

    /*
        usando apenas MPI_Type_vector, o extent fica grande
        organiza manualmente o buffer de envio para combinar com esse extent

        Isso nao é o mais eficiente, mas serve para mostrar a diferenca
        entre usar apenas MPI_Type_vector e usar MPI_Type_create_resized
    */
    if (rank == 0) {
        int p;
        int i;
        int j;

        for (p = 0; p < quantidade_processos; p++) {
            size_t base = (size_t) p * extent_elementos;
            int coluna_inicial = p * colunas_por_processo;

            for (i = 0; i < M; i++) {
                for (j = 0; j < colunas_por_processo; j++) {
                    matriz_envio[base + (size_t) i * N + j] =
                        matriz_A[(size_t) i * N + coluna_inicial + j];
                }
            }
        }
    }

    MPI_Scatter(matriz_envio,1,tipo_colunas,matriz_local,M * colunas_por_processo,MPI_DOUBLE,0,MPI_COMM_WORLD);

    MPI_Scatter(vetor_x,colunas_por_processo,MPI_DOUBLE,vetor_x_local,colunas_por_processo,MPI_DOUBLE,0,MPI_COMM_WORLD);

    double inicio_calculo = MPI_Wtime();

    /*
        Cada processo calcula uma contribuicao parcial para todos os elementos de y.

        Como cada processo tem apenas algumas colunas, ele calcula: y_parcial[i] = soma das colunas que esse processo recebeu
    */
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

    /*
        Agora todos os vetores parciais sao somados no processo 0
        isso forma o vetor y final
    */
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

        printf("\nRESULTADO - MPI_TYPE_VECTOR\n");
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
            printf("Se este teste foi com 1 processo, use o Tempo total como referencia nos proximos testes.\n");
        }
    }

    MPI_Type_free(&tipo_colunas);

    free(matriz_local);
    free(vetor_x_local);
    free(vetor_y_parcial);

    if (rank == 0) {
        free(matriz_A);
        free(matriz_envio);
        free(vetor_x);
        free(vetor_y);
    }

    MPI_Finalize();

    return 0;
}