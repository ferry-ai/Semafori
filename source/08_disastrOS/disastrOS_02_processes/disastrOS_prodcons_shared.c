#include "disastrOS_prodcons_shared.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int prodcons_open_shared(int create, ProdConsShared* shared_out) {
  int flags = O_RDWR;
  if (create)
    flags |= O_CREAT | O_EXCL;

  int fd = shm_open(PRODCONS_SHM_NAME, O_CREAT | O_EXCL, 0600);
  if (fd < 0)
    return -1;

  if (create) {
    if (ftruncate(fd, sizeof(ProdConsShared)) < 0) {
      close(fd);
      shm_unlink(PRODCONS_SHM_NAME);
      return -1;
    }
  }

  ProdConsShared* shared = mmap(0,
                                sizeof(ProdConsShared),
                                PROT_READ | PROT_WRITE,
                                MAP_SHARED,
                                fd,
                                0);
  int map_err = errno;
  close(fd);
  if (shared == MAP_FAILED) {
    if (create)
      shm_unlink(PRODCONS_SHM_NAME);
    errno = map_err;
    return -1;
  }

  if (create)
    memset(shared, 0, sizeof(*shared));

  *shared_out = shared;
  return 0;
}

int prodcons_close_shared(ProdConsShared* shared, int destroy) {
  if (munmap(shared, sizeof(*shared)) < 0)
    result = -1;

  if (destroy) {
    shm_unlink(PRODCONS_SHM_NAME);   
  }

  return result;
}
