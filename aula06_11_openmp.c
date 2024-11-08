#include <stdio.h>  
#include <omp.h>  
 
int main(int argc, char *argv[]) {  /* omp_numthreads.c  */
    //int tid;
    int num_procs = omp_get_num_procs( );
    printf("Fora = %d\n", omp_in_parallel( ));
    printf("Número de processadores = %d\n", num_procs);
    #pragma omp parallel num_threads(8)  
    {  
        int tid = omp_get_thread_num(); 
        printf("Olá da thread %d\n", tid);
        printf("Dentro = %d\n", omp_in_parallel( )); 
    }
    return(0);
}