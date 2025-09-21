#include <stdio.h>

#include "disastrOS.h"
#include "disastrOS_semaphores.h"

#define BUF_SIZE       8
#define N_PRODUCERS    1
#define N_CONSUMERS    2
#define ITEMS_PER_PROD 2
#define STOP_TOKEN    -1

#define SEM_EMPTY_ID  1001
#define SEM_FILL_ID   1002
#define SEM_MUTEX_ID  1003

static int buffer[BUF_SIZE];
static int w_idx = 0, r_idx = 0;

static void log_status(const char* label, int id, const char* message) {
  if (id >= 0)
    printf("[%s%d] %s\n", label, id, message);
  else
    printf("[%s] %s\n", label, message);
  disastrOS_printStatus();
}

static inline void buf_put(int v) {
  buffer[w_idx] = v;
  w_idx = (w_idx + 1) % BUF_SIZE;
}

static inline int buf_get(void) {
  int v = buffer[r_idx];
  r_idx = (r_idx + 1) % BUF_SIZE;
  return v;
}

void idle_task(void* args) {
  (void) args;
  while (1) {
    disastrOS_preempt();
  }
}

void producer_task(void* args) {
  int id = (int)(long) args;

  int empty_fd = disastrOS_semOpen(SEM_EMPTY_ID, 0);
  int fill_fd  = disastrOS_semOpen(SEM_FILL_ID,  0);
  int mutex_fd = disastrOS_semOpen(SEM_MUTEX_ID, 0);

  log_status("P", id, "started");

  for (int i = 0; i < ITEMS_PER_PROD; ++i) {
    printf("[P%d] waiting on SEM_EMPTY\n", id);
    disastrOS_semWait(empty_fd);
    log_status("P", id, "resumed after SEM_EMPTY");

    printf("[P%d] waiting on SEM_MUTEX\n", id);
    disastrOS_semWait(mutex_fd);
    log_status("P", id, "entered critical section");

    int item = id * 1000 + i;
    buf_put(item);
    printf("[P%d] put %d\n", id, item);

    disastrOS_semPost(mutex_fd);
    disastrOS_semPost(fill_fd);
    log_status("P", id, "released mutex and signaled fill");

    disastrOS_sleep(40);
  }

  for (int c = 0; c < N_CONSUMERS; ++c) {
    printf("[P%d] waiting on SEM_EMPTY to send STOP\n", id);
    disastrOS_semWait(empty_fd);
    printf("[P%d] waiting on SEM_MUTEX to send STOP\n", id);
    disastrOS_semWait(mutex_fd);

    log_status("P", id, "entered critical section for STOP");
    buf_put(STOP_TOKEN);
    printf("[P%d] put STOP\n", id);
    disastrOS_semPost(mutex_fd);
    disastrOS_semPost(fill_fd);
    log_status("P", id, "released mutex and signaled STOP");
  }

  log_status("P", id, "exiting");
  disastrOS_exit(0);
}

void consumer_task(void* args) {
  int id = (int)(long) args;

  int empty_fd = disastrOS_semOpen(SEM_EMPTY_ID, 0);
  int fill_fd  = disastrOS_semOpen(SEM_FILL_ID,  0);
  int mutex_fd = disastrOS_semOpen(SEM_MUTEX_ID, 0);

  log_status("C", id, "started");

  while (1) {
    printf("  [C%d] waiting on SEM_FILL\n", id);
    disastrOS_semWait(fill_fd);
    printf("  [C%d] waiting on SEM_MUTEX\n", id);
    disastrOS_semWait(mutex_fd);
    disastrOS_sleep(20);

    log_status("C", id, "entered critical section");

    int item = buf_get();

    disastrOS_semPost(mutex_fd);
    disastrOS_semPost(empty_fd);
    log_status("C", id, "released mutex and signaled empty");

    if (item == STOP_TOKEN) {
      printf("  [C%d] got STOP, bye\n", id);
      log_status("C", id, "received STOP and exits");
      disastrOS_sleep(40);
      break;
    }

    printf("  [C%d] got %d\n", id, item);
    disastrOS_sleep(45);
  }

  log_status("C", id, "exiting");
  disastrOS_exit(0);
}

void init_task(void* args) {
  int empty_fd = disastrOS_semOpen(SEM_EMPTY_ID, BUF_SIZE);
  int fill_fd  = disastrOS_semOpen(SEM_FILL_ID,  0);
  int mutex_fd = disastrOS_semOpen(SEM_MUTEX_ID, 1);

  

  w_idx = r_idx = 0;

  log_status("INIT", -1, "started");

  disastrOS_spawn(idle_task, 0);
  log_status("INIT", -1, "spawned idle task");

  for (int c = 0; c < N_CONSUMERS; ++c) {
    disastrOS_spawn(consumer_task, (void*)(long)c);
    char msg[64];
    snprintf(msg, sizeof(msg), "spawned consumer %d", c);
    log_status("INIT", -1, msg);
  }

  for (int p = 0; p < N_PRODUCERS; ++p) {
    disastrOS_spawn(producer_task, (void*)(long)p);
    char msg[64];
    snprintf(msg, sizeof(msg), "spawned producer %d", p);
    log_status("INIT", -1, msg);
  }

  int alive = N_PRODUCERS + N_CONSUMERS;
  int retval = 0;
  while (alive > 0) {
    int pid = disastrOS_wait(0, &retval);
    if (pid < 0)
      break;
    char msg[128];
    snprintf(msg, sizeof(msg), "child %d terminated, retval:%d, alive before dec:%d",
             pid, retval, alive);
    log_status("INIT", -1, msg);
    --alive;
  }

  log_status("INIT", -1, "shutdown");
  disastrOS_shutdown();
}

int main(int argc, char** argv) {
  (void) argc;
  (void) argv;
  disastrOS_start(init_task, 0, 0);
  return 0;
}
