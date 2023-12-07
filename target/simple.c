#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  char buf[10];
  FILE *fp = fopen(argv[1], "r");
  if (fp == NULL) {
    perror("No file");
    exit(0);
  }
 
  fread(buf, 1, 9, fp);

  if (buf[0] == 'A') {
    if (buf[1] == 'B') {
      if (buf[2] == 'C') {
        __asm__("int $11");
      }
    }
  }
  fclose(fp);
  return 0;
}
