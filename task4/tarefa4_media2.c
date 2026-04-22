#include <stdio.h> 
#include <stdlib.h> 
#include <omp.h> // OpenMP
#include <time.h> 
#include <math.h>

#define N 100000000 // pra alcançar a memória RAM e testar o memory bound

int main() {
    // Criar as variaveis
    double resultado = 0.0;

    // Execução pra pegar o valor de referencia
    double tempo_inicio_p2_ref = omp_get_wtime();
    for (int j=0; j<N; j++) {
        resultado += sin(j) * cos(j) + sqrt(j);
    }
    double tempo_fim_p2_ref = omp_get_wtime();
    double delta_tempo_p2_ref = tempo_fim_p2_ref - tempo_inicio_p2_ref;

    printf("Tempo de referencia: %f seg\n", delta_tempo_p2_ref);


    return 0;
}