<h1 align="center"> TAREFA 19</h1>

## 1. ENUNCIADO
Faça os exercícios de adição de vetores, vadd.c, dos slides 27 e 48 do tutorial de programação de GPUs com OpenMP em um dos nós com GPU do NPAD. Compare os tempos de execução apenas na CPU e com o uso da GPU. Reporte seu progresso, apresentando os problemas encontrados e as soluções propostas.



## 2. EXPLICAÇÃO
### 2.1. GPU x CPU
- CPU = tem poucos núcleos (4-64), onde cada núcleo é rápido; bom para tarefas complexas e variadas.
- GPU = milhares de núcleos, onde cada um é simples e especializado. Bom para terefas repetidas.

#### 2.1.1. Offloading
O programa sempre inicia na CPU. Quando quer usar a GPU, a CPU despacha (offloading) parte do trabalho para a GPU. 

CPU prepara os ddos -> envia para a GPU -> GPU processa -> devolve resultado para a CPU

#### 2.1.2. `pragma omp target`
Essa diretriz que permite a realização do offloading.

Estrutura básica:

`#pragma omp target map(to: a[0:N], b[0:N]) map(from: c[0:N])`

`{`
    
    #pragma omp parallel for
  

`}`
- Tudo o que estiver dentro das chaves após o `target` será executado na GPU;
- `map(to: a[0:N], b[0:N])`: enviar os dados da CPU para o GPU antes de começar, o array "a", do índice 0 até N;
- `map(from: c[0:N])`: ao finalizar, trazer os resultados para a CPU;
- `#pragma omp parallel for`: dentro do target, para que as milhares de threads da GPU possam dividir o trabalho.

Existe um custo para transferir a memória do CPU para o GPU e vice-versa, por isso às vezes pode parecer que a GPU seja mais lenta que a CPU em problemas pequenos.

#### 2.1.3. `pragma omp loop`
Essa diretriz tem a mesma função do `pragma omp parallel for`, com a diferença na maneira que pede ao compilador para fazer essa otimzação.
- for: a pessoa especifica como paralelizar;
- loop: o compilador decide a melhor forma de fazer essa paralelizaçõ.

A GPU é dividida em hierarquia interna de threads, e a diretriz com loop já foi criada pensando nisso, por isso consegue organizar as threads de forma mais eficiente para a GPU específica.

### 2.2. COMPILAR
Compilar no NPAD
`nvc -fast -mp=gpu vadd.c`

#### 2.2.1. Submeter e ver o resultado
`sbatch vadd.sh` <br>
`squeue -u $USER` <br>
`cat vadd-<id>.out`<br>

- `nvc`: compilador do Nvidia (em vez do gcc comum)
- `-fast`: otimização geral de performance
- `mp=gpu`: ativa o suporte openmp com offloading para GPU

## 3. EXPLICAÇÃO DA TAREFA
Verificação do tempo de processamento nas versões com CPU original, as versões GPU com `target` e `for`, e a versão com `target` e `loop`.

## 4. COMO RESOLVER
1º. Manter o loop CPU original e analisar o tempo;
2º. Versão GPU com `#pragma omp target` + `#pragma omp parallel for`, medir tempo;
3º. Versão GPU com `#pragma omp target` + `#pragma omp loop`, medir tempo;

## 5. CONCLUSÃO
Os resultdos mostraram que, pra esse problema específico de adição de vetores com 10 milhões de elementos, a CPU foi mais rápida que a GPU.
- CPU: 0.009s
- GPU parallel for: 0.567s
- GPU loop: 0.878s

### 5.1. CPU > GPU
O tempo de transferência de dados dominou o tempo de execução, isto é, o custo de comunicação superou o ganho do paralelismo da GPU.

### 5.2. GPU for > GPU loop
Na versão com loop, o compilador pode ter adicionado uma camada de organização desnecessária que custou tempo e por isso ficou com tempo maior que a versão com for.