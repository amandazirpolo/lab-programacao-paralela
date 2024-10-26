#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

int main(int argc, char *argv[]) { /* mpi_isend.c  */
int i, meu_ranque, num_procs;
int pot2, destino, meu_valor;
int reducao, recebido, etiq=1, cont=1;
MPI_Status estado;
MPI_Request pedido_envia;
MPI_Request pedido_recebe;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &meu_ranque);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    /* Aborta se número de processos não for potência de 2 */
    pot2 = num_procs;
    while (((pot2 % 2) == 0) && pot2 > 1)
        pot2 /= 2;
    if (pot2 != 1) {
        if (meu_ranque == 0)
           printf("Por favor execute com número de processos potencia de 2 \n");
        MPI_Finalize();
        exit(0);
        }
    /* Cada processo tem um valor diferente para a redução */
    meu_valor = meu_ranque*num_procs;
    reducao = meu_valor;
    /* Realiza a troca de mensagens no padrão do algoritmo de "recursive doubling" */
    for (i = 1; i <= (num_procs/2); i += i) {
        if ((meu_ranque/i)%2 == 0) 
            destino = meu_ranque + i;
        else 
            destino = meu_ranque-i;
    /* Posta os envios e recepções em qualquer ordem */
        MPI_Isend(&reducao, cont, MPI_INT, destino, etiq, MPI_COMM_WORLD, &pedido_envia);
        MPI_Irecv(&recebido, cont, MPI_INT, destino, etiq, MPI_COMM_WORLD, &pedido_recebe); // irecv não bloqueante
    /* As rotinas de "MPI_Wait" asseguram que os dados já foram transmitidos e recebidos */
        MPI_Wait(&pedido_envia, &estado); //  sem wait retorna lixo
        MPI_Wait(&pedido_recebe, &estado); // sem wait retorna lixo
    /* Realiza operação de redução com os dados recebidos */
        if (recebido > reducao)
            reducao = recebido;
    }
    printf("Meu valor = %d, redução = %d \n", meu_valor, reducao);
    MPI_Finalize();
    return(0);
}

/*irecv - wait - isent - wait -> resulta em deadlock*/
/*mpi buffer_attach -> reserva uma área de memória particular externa a área de memória do sistema*/
/*mpi bsend -> o dado é copiado para o buffer de memória reservado e não para o buffer do sistema. assim não corre risco de ser bloqueado por falta de memória*/
/*mpi ssend -> bloqueante sincrono, ou seja, so executa quando o processo é recebido pelo receptor*/
/*mpi rsend -> se o receptor tiver livre, recebe. se não, o dado é jogado fora*/
/*mpi_pack_size -> retorna o tam do que vc quer enviar*/