#pragma once
#include "linked_list.h"
#include "disastrOS_pcb.h"

struct SemDescriptor;
struct SemDescriptorPtr;

// Struttura del semaforo
typedef struct Sem {
  ListItem list;          // nodo per inserirlo nella lista globale dei semafori
  int id;                 // identificatore del semaforo
  int count;              // valore corrente del semaforo
  ListHead descriptors;   // lista dei descriptor che fanno riferimento a questo semaforo
  ListHead waiting_list;  // lista dei processi bloccati su questo semaforo
} Sem;

// Descriptor che rappresenta l'utilizzo del semaforo da parte di un processo
typedef struct SemDescriptor {
  ListItem list;   // nodo per le liste nel PCB e nel Sem
  PCB* pcb;        // processo proprietario
  Sem* sem;        // semaforo referenziato
  int fd;          // identificatore (simile a un file descriptor)
} SemDescriptor;

// Struttura ponte usata per inserire un descriptor in più liste
typedef struct SemDescriptorPtr {
  ListItem list;                // nodo per l'inserimento in liste
  SemDescriptor* descriptor;   // puntatore al descriptor reale
} SemDescriptorPtr;

// Lista globale di tutti i semafori attivi
extern ListHead semaphores_list;

// Inizializza la lista e gli allocator dei semafori
void Sem_init(void);

// Alloca un nuovo semaforo
Sem* Sem_alloc(int id, int count);

// Libera un semaforo (solo se non ha descriptor o processi in attesa)
int Sem_free(Sem* s);

// Alloca un SemDescriptor
SemDescriptor* SemDescriptor_alloc(int fd, Sem* sem, PCB* pcb);

// Libera un SemDescriptor
int SemDescriptor_free(SemDescriptor* d);

// Alloca un SemDescriptorPtr
SemDescriptorPtr* SemDescriptorPtr_alloc(SemDescriptor* descriptor);

// Libera un SemDescriptorPtr
int SemDescriptorPtr_free(SemDescriptorPtr* d);

// Helpers
Sem* Sem_byId(int id);
SemDescriptor* SemDescriptor_byFd(PCB* pcb, int fd);
SemDescriptorPtr* SemDescriptorPtr_findByDescriptor(ListHead* lh, SemDescriptor* d);
