#include <errno.h>
#include <stdio.h>
#include <time.h>

#include "disastrOS.h"
#include "disastrOS_constants.h"
#include "disastrOS_semaphores.h"


#define BUF_SIZE       8
#define N_PRODUCERS    4
#define N_CONSUMERS    3
#define ITEMS_PER_PROD 10
#define STOP_TOKEN    -1

// IDs globali per i semafori (valgono per tutto il processo disastrOS)
#define SEM_EMPTY_ID  1001
#define SEM_FILL_ID   1002
#define SEM_MUTEX_ID  1003

// buffer condiviso (stesso spazio di indirizzi per tutti i PCB in disastrOS)
static int buffer[BUF_SIZE];
static int w_idx = 0, r_idx = 0;
static int stops_remaining = 0;
static int producers_left = 0;

static inline void buf_put(int v) {
  buffer[w_idx] = v;
  w_idx = (w_idx + 1) % BUF_SIZE;
}
static inline int buf_get(void) {
  int v = buffer[r_idx];
  r_idx = (r_idx + 1) % BUF_SIZE;
  return v;
}

static void sleep_ticks(int ticks) {
  if (ticks <= 0) {
    disastrOS_preempt();
    return;
  }

  struct timespec req = {
    .tv_sec = INTERVAL / 1000,
    .tv_nsec = (INTERVAL % 1000) * 1000000L,
  };

  for (int i = 0; i < ticks; ++i) {
    struct timespec rem = req;
    while (nanosleep(&rem, &rem) == -1 && errno == EINTR) {
      // keep sleeping for the remaining time if interrupted by the timer signal
    }
    disastrOS_preempt();
  }
}

void producer_task(void* args) {
  int id = (int)(long) args;

  // Ogni PCB deve aprire i semafori per ottenere i **fd** propri
  int empty_fd = disastrOS_semOpen(SEM_EMPTY_ID, 0);
  int fill_fd  = disastrOS_semOpen(SEM_FILL_ID,  0);
  int mutex_fd = disastrOS_semOpen(SEM_MUTEX_ID, 0);
   //controllo compatto
  if (empty_fd < 0 || fill_fd < 0 || mutex_fd < 0) {
    printf("[P%d] semOpen failed\n", id);
    disastrOS_exit(-1);
  }

  for (int i=0; i<ITEMS_PER_PROD; ++i) {
    disastrOS_semWait(empty_fd);
    disastrOS_semWait(mutex_fd);

    int item = id*1000 + i; // payload di test
    buf_put(item);
    printf("[time=%d] [P%d] put %d\n", disastrOS_time, id, item);

    disastrOS_semPost(mutex_fd);
    disastrOS_semPost(fill_fd);

    sleep_ticks(1); // solo per far vedere il blocco
  }

  disastrOS_semWait(mutex_fd);
  int last = (--producers_left == 0);
  disastrOS_semPost(mutex_fd);

  if (last) {
    disastrOS_semWait(empty_fd);
    disastrOS_semWait(mutex_fd);
    buf_put(STOP_TOKEN);
    printf("[time=%d] [P%d] put STOP\n", disastrOS_time, id);
    disastrOS_semPost(mutex_fd);
    disastrOS_semPost(fill_fd);
  }

  disastrOS_exit(0);
}

void consumer_task(void* args) {
  int id = (int)(long) args;

  int empty_fd = disastrOS_semOpen(SEM_EMPTY_ID, 0);
  int fill_fd  = disastrOS_semOpen(SEM_FILL_ID,  0);
  int mutex_fd = disastrOS_semOpen(SEM_MUTEX_ID, 0);
   //controllo compatto
  if (empty_fd < 0 || fill_fd < 0 || mutex_fd < 0) {
    printf("  [C%d] semOpen failed\n", id);
    disastrOS_exit(-1);
  }

  while (1) {
    disastrOS_semWait(fill_fd);
    disastrOS_semWait(mutex_fd);

    int item = buf_get();

    if (item == STOP_TOKEN) {
      --stops_remaining;
      int more = stops_remaining > 0;
      if (more)
        buf_put(STOP_TOKEN);

      disastrOS_semPost(mutex_fd);
      if (more)
        disastrOS_semPost(fill_fd);
      else
        disastrOS_semPost(empty_fd);

      printf("[time=%d]   [C%d] got STOP, bye\n", disastrOS_time, id);
      break;
    }

    disastrOS_semPost(mutex_fd);
    disastrOS_semPost(empty_fd);

    printf("[time=%d]   [C%d] got %d\n", disastrOS_time, id, item);
    sleep_ticks(2); // per visualizzare l'interleaving
  }

  disastrOS_exit(0);
}

void init_task(void* args) {
  // Crea i semafori una sola volta con i conteggi iniziali corretti
  int empty_fd = disastrOS_semOpen(SEM_EMPTY_ID, BUF_SIZE); // posti liberi
  int fill_fd  = disastrOS_semOpen(SEM_FILL_ID,  0);        // elementi presenti
  int mutex_fd = disastrOS_semOpen(SEM_MUTEX_ID, 1);        // mutua esclusione
  //controllo compatto
  if (empty_fd < 0 || fill_fd < 0 || mutex_fd < 0) {
    printf("[init] semOpen failed\n");
    disastrOS_shutdown();
    return;
  }


  w_idx = r_idx = 0;
  stops_remaining = N_CONSUMERS;
  producers_left = N_PRODUCERS;

  // Spawna consumers e producers (N e M a piacere)
  for (int c=0; c<N_CONSUMERS; ++c)
    disastrOS_spawn(consumer_task, (void*)(long)c);

  for (int p=0; p<N_PRODUCERS; ++p)
    disastrOS_spawn(producer_task, (void*)(long)p);

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

int main(int argc, char** argv) {
  (void)argc; (void)argv;
  disastrOS_start(init_task, 0, 0);
  return 0;
}
