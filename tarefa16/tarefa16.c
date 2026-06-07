#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

// essa função vê se o número é primo ou não
int eh_primo(long long n) {
    if (n < 2) {
        return 0;
    }

    if (n == 2) {
        return 1;
    }

    if (n % 2 == 0) {
        return 0;
    }

    // aqui eu testo só os ímpares, pq os pares já foram tirados
    for (long long i = 3; i <= n / i; i += 2) {
        if (n % i == 0) {
            return 0;
        }
    }

    return 1;
}

// essa função conta quantos primos tem entre inicio e fim
long long contar_primos(long long inicio, long long fim) {
    long long contador = 0;

    for (long long i = inicio; i <= fim; i++) {
        if (eh_primo(i)) {
            contador++;
        }
    }

    return contador;
}

int main(int argc, char *argv[]) {
    int rank, total_processos;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &total_processos);

    // valores padrão caso eu rode sem passar nada no terminal
    long long limite = 1000000;
    long long tamanho_tarefa = 10000;

    // se passar o limite no terminal, ele troca o valor padrão
    if (argc >= 2) {
        limite = atoll(argv[1]);
    }

    // se passar o tamanho da tarefa no terminal, ele troca também
    if (argc >= 3) {
        tamanho_tarefa = atoll(argv[2]);
    }

    // precisa ter pelo menos 1 líder e 1 trabalhador
    if (total_processos < 2) {
        if (rank == 0) {
            printf("Erro: precisa rodar com pelo menos 2 processos.\n");
            printf("Exemplo: mpiexec -n 2 tarefa16.exe 1000000 10000\n");
        }

        MPI_Finalize();
        return 1;
    }

    // rank 0 vai ser o líder
    if (rank == 0) {
        long long total_seq = 0;
        long long total_par = 0;

        double inicio_seq, fim_seq, tempo_seq;
        double inicio_par, fim_par, tempo_par;

        // primeiro faço a versão sequencial pra ter comparação
        inicio_seq = MPI_Wtime();

        total_seq = contar_primos(2, limite);

        fim_seq = MPI_Wtime();
        tempo_seq = fim_seq - inicio_seq;

        // agora começa a parte paralela
        inicio_par = MPI_Wtime();

        long long proximo_inicio = 2;
        int trabalhadores_ativos = 0;

        // manda uma primeira tarefa pra cada trabalhador
        for (int trabalhador = 1; trabalhador < total_processos; trabalhador++) {
            long long tarefa[2];

            if (proximo_inicio <= limite) {
                tarefa[0] = proximo_inicio;
                tarefa[1] = proximo_inicio + tamanho_tarefa - 1;

                if (tarefa[1] > limite) {
                    tarefa[1] = limite;
                }

                // envia a tarefa para o trabalhador
                MPI_Send(tarefa, 2, MPI_LONG_LONG, trabalhador, 0, MPI_COMM_WORLD);

                proximo_inicio = tarefa[1] + 1;
                trabalhadores_ativos++;
            } else {
                // se não tiver mais tarefa, já manda parar
                tarefa[0] = -1;
                tarefa[1] = -1;

                MPI_Send(tarefa, 2, MPI_LONG_LONG, trabalhador, 0, MPI_COMM_WORLD);
            }
        }

        // enquanto tiver trabalhador fazendo alguma coisa
        while (trabalhadores_ativos > 0) {
            MPI_Status status;
            long long resposta;

            // recebe resultado de qualquer trabalhador que terminar primeiro
            MPI_Recv(&resposta, 1, MPI_LONG_LONG, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);

            int trabalhador_livre = status.MPI_SOURCE;

            total_par += resposta;

            long long tarefa[2];

            if (proximo_inicio <= limite) {
                tarefa[0] = proximo_inicio;
                tarefa[1] = proximo_inicio + tamanho_tarefa - 1;

                if (tarefa[1] > limite) {
                    tarefa[1] = limite;
                }

                // manda uma nova tarefa pro trabalhador que acabou de terminar
                MPI_Send(tarefa, 2, MPI_LONG_LONG, trabalhador_livre, 0, MPI_COMM_WORLD);

                proximo_inicio = tarefa[1] + 1;
            } else {
                // se não tiver mais nada, manda ele parar
                tarefa[0] = -1;
                tarefa[1] = -1;

                MPI_Send(tarefa, 2, MPI_LONG_LONG, trabalhador_livre, 0, MPI_COMM_WORLD);

                trabalhadores_ativos--;
            }
        }

        fim_par = MPI_Wtime();
        tempo_par = fim_par - inicio_par;

        int trabalhadores = total_processos - 1;

        long long qtd_tarefas = 0;
        if (limite >= 2) {
            qtd_tarefas = ((limite - 2 + 1) + tamanho_tarefa - 1) / tamanho_tarefa;
        }

        double speedup = tempo_seq / tempo_par;
        double eficiencia = (speedup / trabalhadores) * 100.0;

        printf("\nRESULTADO\n");
        printf("Limite: %lld\n", limite);
        printf("Tamanho da tarefa: %lld\n", tamanho_tarefa);
        printf("Quantidade de tarefas: %lld\n", qtd_tarefas);
        printf("Processos MPI: %d\n", total_processos);
        printf("Trabalhadores: %d\n", trabalhadores);

        printf("\nVERSAO SEQUENCIAL\n");
        printf("Primos encontrados: %lld\n", total_seq);
        printf("Tempo: %f segundos\n", tempo_seq);

        printf("\nVERSAO PARALELA MPI\n");
        printf("Primos encontrados: %lld\n", total_par);
        printf("Tempo: %f segundos\n", tempo_par);

        printf("\nCOMPARACAO\n");
        printf("Resultados iguais? %s\n", total_seq == total_par ? "SIM" : "NAO");
        printf("Speedup: %.2fx\n", speedup);
        printf("Eficiencia: %.2f%%\n", eficiencia);
    }

    // todos os outros ranks são trabalhadores
    else {
        while (1) {
            long long tarefa[2];

            // trabalhador fica esperando o líder mandar tarefa
            MPI_Recv(tarefa, 2, MPI_LONG_LONG, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // se recebeu -1, é pq acabou tudo
            if (tarefa[0] == -1) {
                break;
            }

            long long inicio = tarefa[0];
            long long fim = tarefa[1];

            // calcula a quantidade de primos da tarefa recebida
            long long qtd_primos = contar_primos(inicio, fim);

            // manda o resultado de volta pro líder
            MPI_Send(&qtd_primos, 1, MPI_LONG_LONG, 0, 1, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();

    return 0;
}