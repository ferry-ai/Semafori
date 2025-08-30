#include <assert.h>

#include "disastrOS_semaphores.h"
#include "pool_allocator.h"
#include "disastrOS_constants.h"

#define SEM_SIZE sizeof(Sem)
#define SEM_MEMSIZE (sizeof(Sem)+sizeof(int))
#define MAX_NUM_SEMAPHORES 1024
#define SEM_BUFFER_SIZE MAX_NUM_SEMAPHORES*SEM_MEMSIZE

static char _sem_buffer[SEM_BUFFER_SIZE];
static PoolAllocator _sem_allocator;

#define SEM_DESCRIPTOR_SIZE sizeof(SemDescriptor)
#define SEM_DESCRIPTOR_MEMSIZE (sizeof(SemDescriptor)+sizeof(int))
#define MAX_NUM_SEM_DESCRIPTORS_PER_PROCESS 32
#define MAX_NUM_SEM_DESCRIPTORS (MAX_NUM_SEM_DESCRIPTORS_PER_PROCESS*MAX_NUM_PROCESSES)
#define SEM_DESCRIPTOR_BUFFER_SIZE MAX_NUM_SEM_DESCRIPTORS*SEM_DESCRIPTOR_MEMSIZE

static char _sem_descriptor_buffer[SEM_DESCRIPTOR_BUFFER_SIZE];
static PoolAllocator _sem_descriptor_allocator;

#define SEM_DESCRIPTOR_PTR_SIZE sizeof(SemDescriptorPtr)
#define SEM_DESCRIPTOR_PTR_MEMSIZE (sizeof(SemDescriptorPtr)+sizeof(int))
#define SEM_DESCRIPTOR_PTR_BUFFER_SIZE MAX_NUM_SEM_DESCRIPTORS*SEM_DESCRIPTOR_PTR_MEMSIZE

static char _sem_descriptor_ptr_buffer[SEM_DESCRIPTOR_PTR_BUFFER_SIZE];
static PoolAllocator _sem_descriptor_ptr_allocator;

ListHead semaphores_list;

void Sem_init(void) {
  List_init(&semaphores_list);

  int result=PoolAllocator_init(&_sem_allocator,
                                SEM_SIZE,
                                MAX_NUM_SEMAPHORES,
                                _sem_buffer,
                                SEM_BUFFER_SIZE);
  assert(!result);

  result=PoolAllocator_init(&_sem_descriptor_allocator,
                            SEM_DESCRIPTOR_SIZE,
                            MAX_NUM_SEM_DESCRIPTORS,
                            _sem_descriptor_buffer,
                            SEM_DESCRIPTOR_BUFFER_SIZE);
  assert(!result);

  result=PoolAllocator_init(&_sem_descriptor_ptr_allocator,
                            SEM_DESCRIPTOR_PTR_SIZE,
                            MAX_NUM_SEM_DESCRIPTORS,
                            _sem_descriptor_ptr_buffer,
                            SEM_DESCRIPTOR_PTR_BUFFER_SIZE);
  assert(!result);
}

Sem* Sem_alloc(int id, int count) {
  Sem* s=(Sem*)PoolAllocator_getBlock(&_sem_allocator);
  if (!s)
    return 0;
  s->list.prev=s->list.next=0;
  s->id=id;
  s->count=count;
  List_init(&s->descriptors);
  List_init(&s->waiting_list);
  return s;
}

int Sem_free(Sem* s) {
  assert(s->descriptors.first==0);
  assert(s->descriptors.last==0);
  assert(s->waiting_list.first==0);
  assert(s->waiting_list.last==0);
  return PoolAllocator_releaseBlock(&_sem_allocator, s);
}

SemDescriptor* SemDescriptor_alloc(int fd, Sem* sem, PCB* pcb) {
  SemDescriptor* d=(SemDescriptor*)PoolAllocator_getBlock(&_sem_descriptor_allocator);
  if (!d)
    return 0;
  d->list.prev=d->list.next=0;
  d->pcb=pcb;
  d->sem=sem;
  d->fd=fd;
  return d;
}

int SemDescriptor_free(SemDescriptor* d) {
  return PoolAllocator_releaseBlock(&_sem_descriptor_allocator, d);
}

SemDescriptorPtr* SemDescriptorPtr_alloc(SemDescriptor* descriptor) {
  SemDescriptorPtr* d=(SemDescriptorPtr*)PoolAllocator_getBlock(&_sem_descriptor_ptr_allocator);
  if (!d)
    return 0;
  d->list.prev=d->list.next=0;
  d->descriptor=descriptor;
  return d;
}

int SemDescriptorPtr_free(SemDescriptorPtr* d) {
  return PoolAllocator_releaseBlock(&_sem_descriptor_ptr_allocator, d);
}
Sem* Sem_byId(int id) {
  ListItem* item = semaphores_list.first;
  while (item) {
    Sem* s = (Sem*) item;
    if (s->id == id)
      return s;
    item = item->next;
  }
  return 0;
}

SemDescriptor* SemDescriptor_byFd(PCB* pcb, int fd) {
  ListItem* item = pcb->sem_descriptors.first;
  while (item) {
    SemDescriptor* d = (SemDescriptor*) item;
    if (d->fd == fd)
      return d;
    item = item->next;
  }
  return 0;
}

SemDescriptorPtr* SemDescriptorPtr_findByDescriptor(ListHead* lh, SemDescriptor* d) {
  ListItem* item = lh->first;
  while (item) {
    SemDescriptorPtr* ptr = (SemDescriptorPtr*) item;
    if (ptr->descriptor == d)
      return ptr;
    item = item->next;
  }
  return 0;
}
