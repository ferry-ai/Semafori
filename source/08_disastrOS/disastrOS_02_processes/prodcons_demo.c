#include <errno.h>
#include <stdio.h>

#include "disastrOS.h"
#include "disastrOS_constants.h"
#include "disastrOS_semapores.h"

#define BUF_SIZE        8
#define N_PRODUCERS     3
#define N_CONSUMERS     2
#define ITEMS_PER_PROD  10
#define STOP_TOKEN     -1

//id globali sem
#define SEM_EMPTY_ID  1001
#define SEM_FILL_ID   1002
#define SEM_MUTEX_ID  1003

//BUFFER CONDIVISO
static int buffer[BUF_SIZE];
static int w_idx=0, r_idx=0;
static int stops_remaining = 0;
static int producers_left = 0;

static void buf_put(int v) {
  buffer[w_idx] = v;
  w_idx = (w_idx + 1) % BUF_SIZE;
}
static int buf_get() {
  int v = buffer[r_idx];
  r_idx = (r_idx + 1) % BUF_SIZE;
  return v;
}

static void ds_sleep(int ticks) {
  for (int i = 0; i < ticks; ++i) {
    disastrOS_preempt();
  }
}

void producer_task(void* args){
    int id = (int)(long)args;

    //apertura semafori da parte del prod    
    int empty_fd = disastrOS_semOpen(SEM_EMPTY_ID, BUF_SIZE); //semaforo per dim buffer
    if(empty_fd < 0){
        printf("Errore durante la crazione empty sem");
        disastrOS_exit(-1);
    }
    int fill_fd = disastrOS_semOpen(SEM_FILL_ID, 0);
    if(fill_fd < 0){
        printf("Errore durante la crazione fill sem");
        disastrOS_exit(-1);
    }
    int mutex_fd = disastrOS_semOpen(SEM_MUTEX_ID, 0);
    if(mutex_fd < 0){
        printf("Errore durante la crazione mut sem");
        disastrOS_exit(-1);
    }

    for(int i=0; i<ITEMS_PER_PROD; ++i){
        disastrOS_semWait(empty_fd);
        disastrOS_semWait(mutex_fd);

        int item = id*100 + i;
        buf_put(item);
        printf("[P%d] put %d\n", id, item);

        disastrOS_semPost(mutex_fd);
        disastrOS_semPost(fill_fd);

        ds_sleep(1);
    }
    //roba per stoppare tutto


    disastrOS_exit(0);
}

//consumatore
void consumer_task(void* args){
    int id = (int)args;
    int empty_fd = disastrOS_semOpen(SEM_EMPTY_ID, 0);
    int fill_fd  = disastrOS_semOpen(SEM_FILL_ID,  0);
    int mutex_fd = disastrOS_semOpen(SEM_MUTEX_ID, 0);
    if (empty_fd < 0 || fill_fd < 0 || mutex_fd < 0) {
    printf("  [C%d] semOpen failed\n", id);
    disastrOS_exit(1);
  }

    while(1){
        disastrOS_semWait(fill_fd);
        disastrOS_semWait(mutex_fd);

        int item = buf_get();

        disastrOS_semPost(mutex_fd);
        disastrOS_semPost(empty_fd);

        ds_sleep(2);
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


    //inizializzazione indici
    w_idx = r_idx = 0;

     // Spawna consumers e producers 
    for (int c=0; c<N_CONSUMERS; ++c)
       disastrOS_spawn(consumer_task, (void*)c); //avviamo la task consumer

    for (int p=0; p<N_PRODUCERS; ++p)
       disastrOS_spawn(producer_task, (void*)p);

    int alive = N_PRODUCERS + N_CONSUMERS;
    int retval = 0;
    while (alive > 0) {
       int child = disastrOS_wait(0, &retval);
       if (child < 0)
          break;
       printf("[init] joined pid %d (retval=%d)\n", child, retval);
       --alive;
  }

    disastrOS_shutdown();

}


int main(int argc, char**argv){
    //iniziamo
    disastrOS_start(init_task, 0, 0);
    return 0;
}
