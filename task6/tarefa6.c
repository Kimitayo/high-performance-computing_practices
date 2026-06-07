/*ENUNCIADO
Implemente em C a estimativa estocástica de π. Paralelize com #pragma omp parallel for e explique o resultado incorreto. Corrija a condição de corrida utilizando o #pragma omp critical e reestruturando com #pragma omp parallel seguido de #pragma omp for e aplicando as cláusulas private, firstprivate, lastprivate e shared. Teste diferentes combinações e explique como cada cláusula afeta o comportamento do programa. Comente também como a cláusula default(none) pode ajudar a tornar o escopo mais claro em programas complexos.
 */

/*
  EXPLICAÇÃO
1. Método Estocástico
A estimativa estocástica de pi é um método que utiliza a geração de números aleatórios para estimar o valor de pi. Pontos são gerados aleatoriamente dentro de um quadrado com um círculo inscrito; a proporção de pontos que caem dentro do círculo em relação ao total de pontos gerados é usada para estimar o valor de pi convergindo para esta relação sendo pi/4.

2. Race Condition
Ao paralelizar tarefas, um problema comum é a condição de corrida, quando múltiplas threads tentam acessar e modificar simultanemanete variáveis dependentes, o que pode trazer erros de registro, com valores erroneos que prejudicam o resultado final da tarefa. Para isso, existem diversas soluções para resolver essa problemática, como:
2.1. #pragma omp critical
Essa diretiva cria uma área dentro da região paralela que permite que apenas uma thread entre e execute o que está dentro do bloco critical, garantindo que a variável compartilhada seja modificada por apenas uma thread. Contudo, mesmo resolvendo o problema, essa diretiva não é a solução mais eficiente, já que as threads ficam esperando para acessar a região crítica, o que pode levar a um desempenho ruim.
2.2. #pragma omp parallel + #pragma omp for
Essas abordagem consegue separar a execução paralela da criação das tarefas em blocos iterativos, em que cada thread é responsável por um subconjunto dos pontos a serem gerados. Isso reduz a necessidade de sincronização já que não há dependencia entre as iterações, pois cada thread trabalha com seus próprios dados locais. Após isso, cada thread fica esperando ao final para fazer a sincronização dos resultados (barreira implícita).

3. Cláusulas de escopo na OpenMP
Essas cláusulas tem o objetivo de definir o comportamento das variáveis dentro de uma região paralela, evitando o risco de condições de corrida e garantindo que cada thread tenha acesso às variáveis corretas.
3.1. Shared
Todas as variáveis acessam a mesma variável compartilhada, o que pode levar a condições de corrida se houver escrita simultânea.
3.2. Private
Cada thread tem sua própria cópia da variável (com lixo de memória como valor inicialmente), evitando conflitos de acesso, mas não compartilhando os resultados entre as threads.
3.3. Firstprivate
Semelhante a private, mas inicializa a cópia da variável com o valor da variável original antes da região paralela.
3.4. Lastprivate
Garante que a última thread a modificar a variável atualize a variável original após a região paralela.
3.5. Reduction
Cada thread calcula sua própria parte da redução (por exemplo, soma, produto) e, ao final, os resultados são combinados de forma segura, evitando condições de corrida.
3.6. Default(none)
Faz com que seja necessário declarar explicitamente o escopo de cada variável, tornando o código mais claro e ajudando a evitar erros de escopo, especialmente em programas complexos onde muitas variáveis estão envolvidas.

4. Flag
gcc -o tarefa6 tarefa6.c -lm -fopenmp
./tarefa6

 */

  /* PASSO-A-PASSO
1. Implementar a versão sequencial do Monte Carlo para π
2. Paralelizar com #pragma omp parallel for e mostrar o resultado errado
3. Explicar a race condition
4. Corrigir com #pragma omp critical e medir o impacto no tempo
5. Reestruturar com #pragma omp parallel + #pragma omp for separados (usando count_local)
6. Testar e explicar shared
7. Testar e explicar private
8. Testar e explicar firstprivate
9. Testar e explicar lastprivate
10. Testar e explicar default(none)
11. Montar a tabela comparativa de resultados
12. Escrever o relatório
   */

#include <stdio.h> 
#include <stdlib.h> 
#include <math.h>  
#include <omp.h>
#include <time.h>

#define N 1000000

int meu_rand(unsigned int *seed) { // gerar seed
    *seed = *seed * 1103515245 + 12345;
    return (*seed >> 16) & 0x7fff;
  }

// versao com apenas pragma omp parallel for
void versao_parallel_for() {
  //contagem de tempo início
  double inicio = omp_get_wtime();

  srand(time(NULL));
  int count = 0;

  // declarar o quadrado de 0 a 1 usando rand()
  #pragma omp parallel for //fazer a paralisação
  for(int i=0; i<N; i++) {
    double x = (double)rand()/RAND_MAX;
    double y = (double)rand()/RAND_MAX;

    if (x*x + y*y <= 1) {
      count++;
    }
  }
  // calculo pro valor de pi
  double pi = (4.0 * count)/N;
  // tempo final
  double fim = omp_get_wtime();

  printf("#pragma omp parallel for -> pi: %.6f\n", pi);
  printf("Tempo: %fs\n\n", fim - inicio);
}


// versao com critical e parallel
void versao_parallel_for_mais_critical() {
    //contagem de tempo início
  double inicio = omp_get_wtime();

  srand(time(NULL));
  int count = 0;

  // declarar o quadrado de 0 a 1 usando rand()
  #pragma omp parallel for //fazer a paralisação
  for(int i=0; i<N; i++) {
    double x = (double)rand()/RAND_MAX;
    double y = (double)rand()/RAND_MAX;

    if (x*x + y*y <= 1) {
      #pragma omp critical 
      {
        count++;
      }
    }
  }

  // calculo pro valor de pi
  double pi = (4.0 * count)/N;
  // tempo final
  double fim = omp_get_wtime();

  printf("#pragma omp parallel for + critical -> pi: %.6f\n", pi);
  printf("Tempo: %fs\n\n", fim - inicio);
}


// versao com parallel + omp for
void versao_parallel_mais_for() {
    //contagem de tempo início
  double inicio = omp_get_wtime();

  srand(time(NULL));
  int count = 0;

  // declarar o quadrado de 0 a 1 usando rand()
  #pragma omp parallel //só abre as threads
  {
    int count_local = 0;

    #pragma omp for // só divide o loop
      for(int i=0; i<N; i++) {
        double x = (double)rand()/RAND_MAX;
        double y = (double)rand()/RAND_MAX;
        
        if (x*x + y*y <= 1) {
          count_local++;
        }
      }
      #pragma omp critical
      {
        count += count_local;
      }
  }

  // calculo pro valor de pi
  double pi = (4.0 * count)/N;
  // tempo final
  double fim = omp_get_wtime();

  printf("#pragma omp parallel + for -> pi: %.6f\n", pi);
  printf("Tempo: %fs\n\n", fim - inicio);
}


// versao com parallel e clausula shared
void versao_parallel_mais_shared() {
    //contagem de tempo início
  double inicio = omp_get_wtime();

  srand(time(NULL));
  double x,y;
  int count = 0;

  // declarar o quadrado de 0 a 1 usando rand()
  #pragma omp parallel shared(x, y, count) // indicar que vai ser compartilhado essas variáveis
  {
    int count_local = 0;

    #pragma omp for // só divide o loop
      for(int i=0; i<N; i++) {
        x = (double)rand()/RAND_MAX;
        y = (double)rand()/RAND_MAX;
        
        if (x*x + y*y <= 1) {
          count_local++;
        }
      }
      #pragma omp critical
      {
        count += count_local;
      }
  }

  // calculo pro valor de pi
  double pi = (4.0 * count)/N;
  // tempo final
  double fim = omp_get_wtime();

  printf("#pragma omp parallel + shared -> pi: %.6f\n", pi);
  printf("Tempo: %fs\n\n", fim - inicio);
}


// versao com parallel e clausula private
void versao_parallel_mais_private() {
    //contagem de tempo início
  double inicio = omp_get_wtime();

  srand(time(NULL));
  double x,y;
  int count = 0;

  // declarar o quadrado de 0 a 1 usando rand()
  #pragma omp parallel private(x, y) // indicar que vai ser privado
  {
    int count_local = 0;

    #pragma omp for // só divide o loop
      for(int i=0; i<N; i++) {
        x = (double)rand()/RAND_MAX;
        y = (double)rand()/RAND_MAX;
        
        if (x*x + y*y <= 1) {
          count_local++;
        }
      }
      #pragma omp critical
      {
        count += count_local;
      }
  }

  // calculo pro valor de pi
  double pi = (4.0 * count)/N;
  // tempo final
  double fim = omp_get_wtime();

  printf("#pragma omp parallel + private -> pi: %.6f\n", pi);
  printf("Tempo: %fs\n\n", fim - inicio);
}


// versao com parallel e clausula first private
void versao_parallel_mais_firstprivate() {
    //contagem de tempo início
  double inicio = omp_get_wtime();

  srand(time(NULL));
  unsigned int seed = 2;
  int count = 0;

  // declarar o quadrado de 0 a 1 usando rand()
  #pragma omp parallel firstprivate(seed) // indicar o first private vai trabalhar usando o seed 2 como base
  {
    int count_local = 0;

    #pragma omp for // só divide o loop
      for(int i=0; i<N; i++) {
        double x = (double)meu_rand(&seed)/0x7fff; //rand_r já que usa a seed passada, maior controle
        double y = (double)meu_rand(&seed)/0x7fff;
        
        if (x*x + y*y <= 1) {
          count_local++;
        }
      }
      #pragma omp critical
      {
        count += count_local;
      }
  }

  // calculo pro valor de pi
  double pi = (4.0 * count)/N;
  // tempo final
  double fim = omp_get_wtime();

  printf("#pragma omp parallel + Firstprivate -> pi: %.6f\n", pi);
  printf("Tempo: %fs\n\n", fim - inicio);
}


// versao com parallel e clausula last private
void versao_parallel_mais_lastprivate() {
    //contagem de tempo início
  double inicio = omp_get_wtime();
  double x, y; // aqui devem ser declarados fora da região paralela

  srand(time(NULL));
  int count = 0;

  // declarar o quadrado de 0 a 1 usando rand()
  #pragma omp parallel 
  {
    int count_local = 0;

    #pragma omp for lastprivate(x,y) //já que quem controla das iterações é o for
      for(int i=0; i<N; i++) {
        x = (double)rand()/RAND_MAX;
        y = (double)rand()/RAND_MAX;
        
        if (x*x + y*y <= 1) {
          count_local++;
        }
      }
      #pragma omp critical
      {
        count += count_local;
      }
  }

  // calculo pro valor de pi
  double pi = (4.0 * count)/N;
  // tempo final
  double fim = omp_get_wtime();

  printf("#pragma omp parallel + Lastprivate -> pi: %.6f\n", pi);
  printf("Último x: %f, Último y: %f\n", x, y);
  printf("Tempo: %fs\n\n", fim - inicio);
}


// versao com parallel e clausula default
void versao_parallel_mais_default() { // versão estruturada com tudo explícito
    //contagem de tempo início
  double inicio = omp_get_wtime();
  double x, y; // aqui devem ser declarados fora da região paralela

  srand(time(NULL));
  int count = 0;

  // declarar o quadrado de 0 a 1 usando rand()
  #pragma omp parallel default(none) shared(count) private(x,y)
  {
    int count_local = 0;

    #pragma omp for
      for(int i=0; i<N; i++) {
        x = (double)rand()/RAND_MAX;
        y = (double)rand()/RAND_MAX;
        
        if (x*x + y*y <= 1) {
          count_local++;
        }
      }
      #pragma omp critical
      {
        count += count_local;
      }
  }

  // calculo pro valor de pi
  double pi = (4.0 * count)/N;
  // tempo final
  double fim = omp_get_wtime();

  printf("#pragma omp parallel + default -> pi: %.6f\n", pi);
  printf("Tempo: %fs\n\n", fim - inicio);
}



int main() {
    versao_parallel_for();
    versao_parallel_for_mais_critical();
    versao_parallel_mais_for();
    versao_parallel_mais_shared();
    versao_parallel_mais_private();
    versao_parallel_mais_firstprivate();
    versao_parallel_mais_lastprivate();
    versao_parallel_mais_default();

    return 0;
}

