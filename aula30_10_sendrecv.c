#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) { /* mpi_sendrecv.c  */
    int meu_ranque, num_procs;
    int dados, dados_locais, aux;
    int destino, etiq=0;
    int envia, cont=1;
    MPI_Status estado;
    const int raiz=0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &meu_ranque);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    /* Se o número de processos não for igual a 2 então aborta */
    if (num_procs != 3) { 
        if (meu_ranque == raiz)
            printf("Por favor execute com apenas 3 processos  \n");
        MPI_Finalize();
        exit(0);
    }
    /* Valor a ser enviado */
    dados = meu_ranque * 2 + 1; // meu ranque determina o numero do processo
    printf("Processo %d: tem dado %d \n", meu_ranque, dados);
    
    /* Determina o destino */
    // 0 faz sendrecv, 1 faz recv e 2 faz send
    if(meu_ranque == 0){
        MPI_Sendrecv(&dados, 1, MPI_INT, 1, etiq, &dados_locais, 1, MPI_INT, 2, etiq, MPI_COMM_WORLD, &estado);
    }    
    if(meu_ranque == 1){
        MPI_Recv(&dados_locais, cont, MPI_INT, 0, etiq, MPI_COMM_WORLD, &estado);
    } 
    if(meu_ranque == 2){ 
        MPI_Send(&dados, cont, MPI_INT, 0, etiq, MPI_COMM_WORLD);
    }

    if(dados < dados_locais) dados = dados_locais;

    // 0 faz sendrecv, 1 faz send e 2 faz recv
    if(meu_ranque == 0){
        MPI_Sendrecv(&dados, 1, MPI_INT, 2, etiq, &dados_locais, 1, MPI_INT, 1, etiq, MPI_COMM_WORLD, &estado);
    }
    if(meu_ranque == 1){
        MPI_Send(&dados, cont, MPI_INT, 0, etiq, MPI_COMM_WORLD);
    }
    if(meu_ranque == 2){
        MPI_Recv(&dados_locais, cont, MPI_INT, 0, etiq, MPI_COMM_WORLD, &estado);
    }

    if(dados < dados_locais) dados = dados_locais;

    printf("Processo %d:  recebeu o  dado: %d\n", meu_ranque, dados_locais);
    if(meu_ranque == 0) printf("Maior valor: %d\n", dados);
    MPI_Finalize();
    return(0);
}