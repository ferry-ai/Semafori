#include <errno.h>
#include <stdio.h>

#include "disastrOS.h"
#include "disastrOS_constants.h"
#include "disastrOS_semapores.h"

#define BUF_SIZE        8
#define N_PRODUCERS     3
#define N_CONSUMERS     2
#define ITEMS_PER_PROD  4

//id globali sem
#define SEM_EMPTY_ID  1001
#define SEM_FILL_ID   1002
#define SEM_MUTEX_ID  1003

//BUFFER CONDIVISO
int buffer[BUF_SIZE];
int w_idx=0, r_idx=0;

void producer_task(void* args){
    //apertura semafori da parte del prod    
    int empty_fd = disastrOS_semOpen(SEM_EMPTY_ID, BUF_SIZE); //semaforo per dim buffer
    if(empty_fd < 0){
        printf("Errore durante la crazione empty sem");
        disastrOS_shutdown();
    }
    int fill_fd = disastrOS_semOpen(SEM_FILL_ID, 0);
    if(fill_fd < 0){
        printf("Errore durante la crazione fill sem");
        disastrOS_shutdown();
    }
    int mutex_fd = disastrOS_semOpen(SEM_MUTEX_ID, 0);
    if(mutex_fd < 0){
        printf("Errore durante la crazione mut sem");
        disastrOS_shutdown();
    }

    for(int i=0; i<ITEMS_PER_PROD; ++i){
        disastrOS_semWait(empty_fd);
        disastrOS_semWait(mutex_fd);

        //CS

        disastrOS_semPost(mutex_fd);
        disastrOS_semPost(fill_fd);

        
    }


    disastrOS_exit(0);
}

//consumatore
void consumer_task(void* args){
    int empty_fd = disastrOS_semOpen(SEM_EMPTY_ID, 0);
    int fill_fd  = disastrOS_semOpen(SEM_FILL_ID,  0);
    int mutex_fd = disastrOS_semOpen(SEM_MUTEX_ID, 0);

    while(1){
        disastrOS_semWait(fill_fd);
        disastrOS_semWait(mutex_fd);

        //CS

        disastrOS_semPost(mutex_fd);
        disastrOS_semPost(empty_fd);
    }
    disastrOS_exit(0);
}

void init_task(void* args){
    //crazione semafori
    int empty_fd = disastrOS_semOpen(SEM_EMPTY_ID, BUF_SIZE); //semaforo per dim buffer
    if(empty_fd < 0){
        printf("Errore durante la crazione empty sem");
        disastrOS_shutdown();
    }
    int fill_fd = disastrOS_semOpen(SEM_FILL_ID, 0);
    if(fill_fd < 0){
        printf("Errore durante la crazione fill sem");
        disastrOS_shutdown();
    }
    int mutex_fd = disastrOS_semOpen(SEM_MUTEX_ID, 0);
    if(mutex_fd < 0){
        printf("Errore durante la crazione mut sem");
        disastrOS_shutdown();
    }



     // Spawna consumers e producers 
    for (int c=0; c<5; ++c)
       disastrOS_spawn(consumer_task, (void*)c); //avviamo la task consumer

    for (int p=0; p<5; ++p)
       disastrOS_spawn(producer_task, (void*)p);

}


int main(int argc, char**argv){
    //iniziamo
    disastrOS_start(init_task, 0, 0);
    return 0;
}
