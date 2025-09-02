#include <stdio.h>
#include <assert.h>

#include "disastrOS.h"
#include "disastrOS_semaphores.h"
#include "disastrOS_globals.h"

void initFunction(void* args) {
  printf("init: testing semaphore basic ops\n");

  printf("\n");
  int sem_fd = disastrOS_semOpen(1, 1);
  assert(sem_fd >= 0);  //verifico apertura andata a buon fine
  disastrOS_semWait(sem_fd);
  disastrOS_semPost(sem_fd);
  int sem_fdb = disastrOS_semOpen(1, 1);
  assert(sem_fd >= 0);  //verifico apertura andata a buon fine
  disastrOS_semClose(sem_fd);
  //il semaforo esiste ancora?
  printf("semaforo esiste? %s \n", Sem_byId(1)!=0?"si":"no");
  disastrOS_semClose(sem_fdb); 
  printf("semaforo esiste? %s \n", Sem_byId(1)!=0?"si":"no"); //dopo aver chiuso anche l'ultimo fd il semaforo è liberato
  printf("\n");
  assert(semaphores_list.first == 0);
  printf("sem open/wait/post/close OK\n \n");

  printf("init: testing cleanup on exit\n");
  int sem_fd2 = disastrOS_semOpen(2, 0);
  assert(sem_fd2 >= 0);
  sem_cleanup_on_exit(running);
  assert(semaphores_list.first == 0);
  printf("init: cleanup OK\n");
}

int main(int argc, char** argv) {
  printf("start\n");
  disastrOS_start(initFunction, 0, 0);
  return 0;
}
