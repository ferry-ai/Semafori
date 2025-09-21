#pragma once

#include "linked_list.h"
#include "disastrOS_pcb.h"

struct SemDescriptor;
struct SemDescriptorPtr;

typedef struct Sem {
  ListItem list;
  int id;
  int count;
  ListHead descriptors;
  ListHead waiting_list;
} Sem;

typedef struct SemDescriptor {
  ListItem list;
  PCB* pcb;
  Sem* sem;
  int fd;
} SemDescriptor;

typedef struct SemDescriptorPtr {
  ListItem list;
  SemDescriptor* descriptor;
} SemDescriptorPtr;

extern ListHead semaphores_list;

void Sem_init(void);
Sem* Sem_alloc(int id, int count);
int Sem_free(Sem* s);

SemDescriptor* SemDescriptor_alloc(int fd, Sem* sem, PCB* pcb);
int SemDescriptor_free(SemDescriptor* d);

SemDescriptorPtr* SemDescriptorPtr_alloc(SemDescriptor* descriptor);
int SemDescriptorPtr_free(SemDescriptorPtr* d);

Sem* Sem_byId(int id);
SemDescriptor* SemDescriptor_byFd(PCB* pcb, int fd);
SemDescriptorPtr* SemDescriptorPtr_findByDescriptor(ListHead* lh, SemDescriptor* d);

void sem_cleanup_on_exit(PCB* pcb);
