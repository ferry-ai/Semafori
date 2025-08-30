#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"
#include "disastrOS_semaphores.h"

void internal_semOpen(){
  int id = running->syscall_args[0];
  int init = running->syscall_args[1];

  if (id < 0) {
    running->syscall_retvalue = DSOS_ESYSCALL_ARGUMENT_OUT_OF_BOUNDS;
    return;
  }

  Sem* sem = Sem_byId(id);
  if (!sem) {
    sem = Sem_alloc(id, init < 0 ? 0 : init);
    if (!sem) {
      running->syscall_retvalue = DSOS_ESYSCALL_OUT_OF_RANGE;
      return;
    }
    List_insert(&semaphores_list, semaphores_list.last, (ListItem*) sem);
  }

  int fd = running->last_sem_fd++;
  SemDescriptor* des = SemDescriptor_alloc(fd, sem, running);
  if (!des) {
    running->syscall_retvalue = DSOS_ESYSCALL_OUT_OF_RANGE;
    return;
  }
  List_insert(&running->sem_descriptors, running->sem_descriptors.last, (ListItem*) des);

  SemDescriptorPtr* des_ptr = SemDescriptorPtr_alloc(des);
  if (!des_ptr) {
    List_detach(&running->sem_descriptors, (ListItem*) des);
    SemDescriptor_free(des);
    running->syscall_retvalue = DSOS_ESYSCALL_OUT_OF_RANGE;
    return;
  }
  List_insert(&sem->descriptors, sem->descriptors.last, (ListItem*) des_ptr);

  running->syscall_retvalue = fd;
}

void internal_semClose(){
  int fd = running->syscall_args[0];
  SemDescriptor* des = SemDescriptor_byFd(running, fd);
  if (!des) {
    running->syscall_retvalue = DSOS_ESYSCALL_ARGUMENT_OUT_OF_BOUNDS;
    return;
  }
  Sem* sem = des->sem;

  List_detach(&running->sem_descriptors, (ListItem*) des);

  SemDescriptorPtr* desc_ptr = SemDescriptorPtr_findByDescriptor(&sem->descriptors, des);
  if (desc_ptr) {
    List_detach(&sem->descriptors, (ListItem*) desc_ptr);
    SemDescriptorPtr_free(desc_ptr);
  }

  SemDescriptor_free(des);

  if (sem->waiting_list.first == 0 && sem->descriptors.first == 0) {
    List_detach(&semaphores_list, (ListItem*) sem);
    Sem_free(sem);
  }

  running->syscall_retvalue = 0;
}

void internal_semWait(){
  running->syscall_retvalue = DSOS_ESYSCALL_NOT_IMPLEMENTED;
  return;
}

void internal_semPost(){
  running->syscall_retvalue = DSOS_ESYSCALL_NOT_IMPLEMENTED;
  return;
}