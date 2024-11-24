#include <stdlib.h>
#include <mpi.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

// função para alocar memória para a matriz e para os vetores
void aloca_matriz_e_vetor(int tam, double ***matriz, double **b, double **x) {
    *matriz = (double **)malloc(tam * sizeof(double *));
    if (!*matriz) {
        printf("Erro ao alocar memória para a matriz\n");
        exit(1);
    }

    for (int i = 0; i < tam; i++) {
        (*matriz)[i] = (double *)malloc(tam * sizeof(double));
        if (!(*matriz)[i]) {
            printf("Erro ao alocar memória para a linha %d\n", i);
            exit(1);
        }
    }

    *b = (double *)malloc(tam * sizeof(double)); // aloca memória para o vetor b
    *x = (double *)malloc(tam * sizeof(double)); // aloca memória para o vetor x
}

// função para gerar matriz dinamicamente e preencher com números aleatórios
void gera_matriz_e_vetor(int tam, double ***matriz, double **b) {
    // preenche a matriz com números aleatórios
    for (int i = 0; i < tam; i++) {
        for (int j = 0; j < tam; j++) {
            (*matriz)[i][j] = (double)(rand() % 10) + 1; // gera números entre 1 e 10
        }
    }

    // inicializa o vetor b com valores aleatórios
    for (int i = 0; i < tam; i++) {
        (*b)[i] = (double) (rand() % 10) + 1;
    }
}

// função para imprimir a matriz
void imprime_matriz(int tam, double **matriz) {
    for (int i = 0; i < tam; i++) {
        for (int j = 0; j < tam; j++) {
            printf("%6.2f ", matriz[i][j]); // imprime com 2 casas decimais
        }
        printf("\n");
    }
}

// função para liberar a memória da matriz
void libera_matriz(int tam, double **matriz) {
    for (int i = 0; i < tam; i++) {
        free(matriz[i]);
    }
    free(matriz);
}

// função para imprimir o vetor b
void imprime_vetor_b(int tam, double *b) {
    for (int i = 0; i < tam; i++) {
        printf("%6.2f ", b[i]);
    }
    printf("\n");
}

// forward elimination - primeira parte do cálculo de gauss
void forward_elimination(double **A, double *b, int n, int rank, int size) {
    // vetor para pegar a linha do "pivot"
    double *linha_pivot = (double *)malloc((n + 1) * sizeof(double)); 

    int rows_per_proc = n / size;
    int resto = n % size; // para garantir que todos os processos terão a mesma quantidade de linhas

    int *num_elementos_processo = (int *)malloc(size * sizeof(int)); // usado para enviar as linhas para os processos
    int *displs = (int *)malloc(size * sizeof(int)); // deslocamento de dados da matriz

    for (int i = 0; i < size; i++) {
        num_elementos_processo[i] = (rows_per_proc + (i < resto ? 1 : 0)) * (n + 1); // divide as linhas entre os processos
        displs[i] = (i == 0) ? 0 : displs[i - 1] + num_elementos_processo[i - 1]; // calcula o deslocamento
    }

    double *linhas = (double *)malloc(num_elementos_processo[rank] * sizeof(double)); // vetor para atualizar as linhas
    double *linhas_calculadas = NULL;

    if (rank == 0) {
        linhas_calculadas = (double *)malloc((n * (n + 1)) * sizeof(double));
        assert(linhas_calculadas != NULL);
    }

    
    for (int k = 0; k < n - 1; k++) {
        if (rank == 0) {
            for (int i = 0; i < n; i++) {
                linha_pivot[i] = A[k][i]; // gera as linhas "pivot"
            }
            linha_pivot[n] = b[k];
        }

        MPI_Bcast(linha_pivot, n + 1, MPI_DOUBLE, 0, MPI_COMM_WORLD); // manda a linha "pivot" para todos os processos

        // junta as linhas da matriz com o vetor b e "lineariza" a matriz
        if (rank == 0) {
            int offset = 0;
            for (int i = k + 1; i < n; i++) {
                for (int j = 0; j < n; j++) { 
                    linhas_calculadas[offset++] = A[i][j];
                }
                linhas_calculadas[offset++] = b[i];
            }
        }

        MPI_Scatterv(linhas_calculadas, num_elementos_processo, displs, MPI_DOUBLE, linhas,
                        num_elementos_processo[rank], MPI_DOUBLE, 0, MPI_COMM_WORLD);

        // elimina as linhas abaixo do "pivot"
        int num_linhas = num_elementos_processo[rank] / (n + 1);
        for (int i = 0; i < num_linhas; i++) {
            double fator = linhas[i * (n + 1) + k] / linha_pivot[k];
            for (int j = k; j < n + 1; j++) {
                linhas[i * (n + 1) + j] -= fator * linha_pivot[j];
            }
        }

        MPI_Gatherv(linhas, num_elementos_processo[rank], MPI_DOUBLE, linhas_calculadas,
                    num_elementos_processo, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        // remonta a matriz e o vetor b
        if (rank == 0) {
            int offset = 0;
            for (int i = k + 1; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    A[i][j] = linhas_calculadas[offset++];
                }
                b[i] = linhas_calculadas[offset++];
            }
        }
    }

    free(linha_pivot);
    free(linhas);
    free(num_elementos_processo);
    free(displs);

    if (rank == 0) {
        free(linhas_calculadas);
    }
}

// back substitution - segunda parte do cálculo de gauss
void back_substitution(double **A, double *b, double *x, int n) {
    for (int i = n - 1; i >= 0; i--) {
        x[i] = b[i]; // começa com o valor de b[i]
        for (int j = i + 1; j < n; j++) {
            x[i] -= A[i][j] * x[j]; // subtrai os valores já calculados
        }
        x[i] /= A[i][i]; // divide pelo pivot para encontrar o valor de x[i]
    }
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    double tempo_inicial = MPI_Wtime();

    int rank, num_procs, tam;
    double **matriz, *b, *x;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    if (rank == 0) {
        printf("Digite o tamanho da matriz: \n");
        scanf("%d", &tam);
    }

    // broadcast do tamanho da matriz para todos os processos
    MPI_Bcast(&tam, 1, MPI_INT, 0, MPI_COMM_WORLD);
    aloca_matriz_e_vetor(tam, &matriz, &b, &x);

    // gera a matriz e o vetor apenas no processo 0 e distribui
    if (rank == 0) {
        gera_matriz_e_vetor(tam, &matriz, &b);
        printf("Matriz gerada:\n");
        imprime_matriz(tam, matriz);
        printf("Vetor b gerado:");
        imprime_vetor_b(tam, b);
    }

    printf("\n\n");

    // forward elimination
    forward_elimination(matriz, b, tam, rank, num_procs);

    MPI_Barrier(MPI_COMM_WORLD); // sincroniza os processos

    // back substitution apenas no processo 0
    if (rank == 0) {
        printf("Matriz final após Forward Elimination:\n");
        imprime_matriz(tam, matriz);
        printf("\nVetor b:");
        imprime_vetor_b(tam, b);
        
        back_substitution(matriz, b, x, tam);
        double tempo_final = MPI_Wtime();

        printf("Solução: ");
        for (int i = 0; i < tam; i++) {
            printf("%6.2f ", x[i]);
        }
        printf("\n");
        printf("Tempo de execução: %f segundos\n", tempo_final - tempo_inicial);
    }

    libera_matriz(tam, matriz);
    free(b);
    free(x);

    MPI_Finalize();
    return 0;
}    