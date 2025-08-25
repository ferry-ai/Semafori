#pragma once
#include <assert.h>
#include "disastrOS.h"
#include "disastrOS_globals.h"

void internal_preempt();

void internal_fork();

void internal_exit();

void internal_wait();

void internal_spawn();

void internal_shutdown();

void internal_semOpen();
void internal_semClose();
void internal_semWait();
void internal_semPost();

void internal_schedule();

// user-level semaphore API
int disastrOS_semOpen(int id, int init);
int disastrOS_semClose(int id);
int disastrOS_semWait(int id);
int disastrOS_semPost(int id);
