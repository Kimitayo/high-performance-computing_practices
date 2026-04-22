#include <stdio.h> 
#include <stdlib.h> 
#include <omp.h> // OpenMP
#include <time.h> 

#define N 100000000 // pra alcançar a memória RAM e testar o memory bound

int main() {
    // Criar os vetores
    double *vetor_soma;
    double *vetor_x;
    double *vetor_y;

    vetor_soma = (double *) malloc( (N)*sizeof(double) );
    vetor_x = (double *) malloc( (N)*sizeof(double) );
    vetor_y = (double *) malloc( (N)*sizeof(double) );

    if (vetor_soma == NULL || vetor_x == NULL || vetor_y == NULL) {
        printf("Erro: Faltou memória!\n");
        return 1;
    } 

    // Programa 1: soma dos vetores

    // Atribuição de valores aos vetores
    for (int i=0; i<N; i++) {
        vetor_x[i] = (double) i;
        vetor_y[i] = (double) i;
    }

// Execução pra pegar o valor de referencia
    double tempo_inicio_p1_ref = omp_get_wtime();
    for (int j=0; j<N; j++) {
        vetor_soma[j] = vetor_x[j] + vetor_y[j];
    }
    double tempo_fim_p1_ref = omp_get_wtime();
    double delta_tempo_p1_ref = tempo_fim_p1_ref - tempo_inicio_p1_ref;

    printf("Tempo de referencia: %f seg\n", delta_tempo_p1_ref);

    free(vetor_soma);
    free(vetor_x);
    free(vetor_y);

    return 0;
}