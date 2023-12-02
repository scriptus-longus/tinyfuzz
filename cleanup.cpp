#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "fuzz.hpp"

int main() {
  int fd = shm_open(STORAGE_ID, O_RDONLY, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    perror("open");
    return -1;
  }

  printf("unlinking\n");
  fd = shm_unlink("/fuzz_map");
  if (fd == -1) {
    perror("unlink"); 
    return -1;
  }

  return 0;
}
