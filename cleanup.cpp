#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "fuzz.hpp"


int main() {
  printf("unlinking\n");
  int segid = shmget(0x1001, SHARED_MEM_SIZE*sizeof(int), IPC_EXCL | S_IRUSR | S_IWUSR);

  shmctl(segid, IPC_RMID, 0);

  return 0;
}
