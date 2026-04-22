#include <stdio.h> 
#include <stdlib.h> 
#include <omp.h> // OpenMP
#include <time.h> 
#include <math.h>

#define N 100000000 // pra alcançar a memória RAM e testar o memory bound
#define DELTA_TEMPO_P2_REF 2.570

int main() {
    // Criar as variaveis
    double resultado = 0.0;
    
    // iniciar contagem de tempo
    double tempo_inicio_p2 = omp_get_wtime();

    // Cálculo pesado
    #pragma omp parallel for reduction(+:resultado) // para as várias threads não ficarem tentando modificar resultados ao mesmo tempo
    for (int j=0; j<N; j++) {
        resultado += sin(j) * cos(j) + sqrt(j);
    }

    // Tempo final
    double tempo_fim_p2 = omp_get_wtime();
    double delta_tempo_p2 = tempo_fim_p2 - tempo_inicio_p2;
    printf("Threads: %d | Tempo: %fseg\n", omp_get_max_threads(), delta_tempo_p2);

    // Cálculo do speedup e eficiecia
    double speedup_p2 = DELTA_TEMPO_P2_REF / delta_tempo_p2;
    double eficiencia_p2 = (speedup_p2 / omp_get_max_threads())*100;
    printf("Speedup: %.2fx | Eficiencia: %.2f%%\n", speedup_p2, eficiencia_p2);


    return 0;
}