#include <stdlib.h>
#include <mpi.h>
#include <stdio.h>

// Função para gerar matriz dinamicamente e preencher com números aleatórios
void gera_matriz(int tam, double ***matriz) {
    // Aloca memória para as linhas
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

    // Preenche a matriz com números aleatórios
    for (int i = 0; i < tam; i++) {
        for (int j = 0; j < tam; j++) {
            (*matriz)[i][j] = (double)(rand() % 11); // Gera números entre 0 e 10
        }
    }
}

// Função para imprimir a matriz
void imprime_matriz(int tam, double **matriz) {
    for (int i = 0; i < tam; i++) {
        for (int j = 0; j < tam; j++) {
            printf("%6.2f ", matriz[i][j]); // Imprime com 2 casas decimais
        }
        printf("\n");
    }
}

// Função para liberar a memória da matriz
void libera_matriz(int tam, double **matriz) {
    for (int i = 0; i < tam; i++) {
        free(matriz[i]);
    }
    free(matriz);
}

// Forward elimination
void forward_elimination(double **A, double *b, int n, int rank, int size) {
    int rows_per_proc = n / size;
    
    for (int k = 0; k < n; k++) {
        // Cada processo obtém o pivô k e o broadcast dele
        double pivot = A[k][k];
        MPI_Bcast(&pivot, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        // Elimina as linhas abaixo do pivô
        for (int i = k + 1 + rank * rows_per_proc; i < n; i += size) {
            if (i >= n) break; // Caso o número de linhas não seja divisível pelo número de processos
            double factor = A[i][k] / pivot;
            for (int j = k; j < n; j++) {
                A[i][j] -= factor * A[k][j]; // Subtrai a linha k da linha i
            }
            b[i] -= factor * b[k]; // Atualiza o vetor b
        }
    }
}

// Back substitution
void back_substitution(double **A, double *b, double *x, int n) {
    for (int i = n - 1; i >= 0; i--) {
        x[i] = b[i]; // Começa com o valor de b[i]
        for (int j = i + 1; j < n; j++) {
            x[i] -= A[i][j] * x[j]; // Subtrai os valores já calculados
        }
        x[i] /= A[i][i]; // Divide pelo pivô para encontrar o valor de x[i]
    }
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    double tempo_inicial = MPI_Wtime();

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double **matriz;
    int tam;

    if (rank == 0) {
        printf("Digite o tamanho da matriz: \n");
        scanf("%d", &tam);
    }

    // Broadcast do tamanho da matriz para todos os processos
    MPI_Bcast(&tam, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Gera matriz apenas no processo 0 e distribui
    if (rank == 0) {
        gera_matriz(tam, &matriz);
        printf("Matriz gerada:\n");
        imprime_matriz(tam, matriz);
    } else {
        matriz = (double **)malloc(tam * sizeof(double *));
        for (int i = 0; i < tam; i++) {
            matriz[i] = (double *)malloc(tam * sizeof(double));
        }
    }

    double *b = (double *)malloc(tam * sizeof(double));
    double *x = (double *)malloc(tam * sizeof(double));

    // Inicializa o vetor b com valores aleatórios
    for (int i = 0; i < tam; i++) {
        b[i] = rand() % 11;
    }

    printf("vetor b:  \n");
    for(int i = 0; i < tam; i++){
        printf("%6.2f", b[i]);
    }
    printf("\n");

    // Forward elimination
    forward_elimination(matriz, b, tam, rank, size);

    // Back substitution apenas no processo 0
    if (rank == 0) {
        back_substitution(matriz, b, x, tam);
        double tempo_final = MPI_Wtime();

        printf("Solução: ");
        for (int i = 0; i < tam; i++) {
            printf("%6.2f ", x[i]);
        }
        printf("\n");
        printf("Tempo de execução: %f segundos\n", tempo_final - tempo_inicial);
    }

    // Libera memória
    libera_matriz(tam, matriz);
    free(b);
    free(x);

    MPI_Finalize();
    return 0;
}
