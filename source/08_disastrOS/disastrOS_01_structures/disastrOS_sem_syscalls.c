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
  int fd = running->syscall_args[0];

  SemDescriptor* des = SemDescriptor_byFd(running, fd);
  if (!des) {
    running->syscall_retvalue = DSOS_ESYSCALL_ARGUMENT_OUT_OF_BOUNDS;
    return;
  }

  Sem* sem = des->sem;

  sem->count--;

  if (sem->count < 0) {
    SemDescriptorPtr* des_ptr = SemDescriptorPtr_findByDescriptor(&sem->descriptors, des);
    if (des_ptr) {
      List_detach(&sem->descriptors, (ListItem*) des_ptr);
      List_insert(&sem->waiting_list, sem->waiting_list.last, (ListItem*) des_ptr);
    }

    
    running->status = Waiting;
    List_insert(&waiting_list, waiting_list.last, (ListItem*) running);

    PCB* next_running = (PCB*) List_detach(&ready_list, ready_list.first);
    if (next_running)
      next_running->status = Running;
    running = next_running;
    return;
  }

  running->syscall_retvalue = 0;
}

void internal_semPost(){
  int fd = running->syscall_args[0];

  SemDescriptor* des = SemDescriptor_byFd(running, fd);
  if (!des) {
    running->syscall_retvalue = DSOS_ESYSCALL_ARGUMENT_OUT_OF_BOUNDS;
    return;
  }

  Sem* sem = des->sem;

  // release a token on the semaphore
  sem->count++;

  // if there are waiting processes wake up one
  if (sem->count <= 0 && sem->waiting_list.first) {
    // get first waiting descriptor
    SemDescriptorPtr* des_ptr = (SemDescriptorPtr*) List_detach(&sem->waiting_list, sem->waiting_list.first);
    if (des_ptr) {
      // reinsert descriptor in active list
      List_insert(&sem->descriptors, sem->descriptors.last, (ListItem*) des_ptr);

      // move the corresponding process to ready state
      PCB* pcb = des_ptr->descriptor->pcb;
      List_detach(&waiting_list, (ListItem*) pcb);
      pcb->status = Ready;
      pcb->syscall_retvalue = 0;
      List_insert(&ready_list, ready_list.last, (ListItem*) pcb);
    }
  }

  running->syscall_retvalue = 0;
}