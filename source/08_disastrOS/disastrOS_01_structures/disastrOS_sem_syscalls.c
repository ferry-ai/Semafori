#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"
#include "disastrOS_semaphores.h"

void internal_semOpen(){
  running->syscall_retvalue = DSOS_ESYSCALL_NOT_IMPLEMENTED;
  return;
}

void internal_semClose(){
  running->syscall_retvalue = DSOS_ESYSCALL_NOT_IMPLEMENTED;
  return;
}

void internal_semWait(){
  running->syscall_retvalue = DSOS_ESYSCALL_NOT_IMPLEMENTED;
  return;
}

void internal_semPost(){
  running->syscall_retvalue = DSOS_ESYSCALL_NOT_IMPLEMENTED;
  return;
}

