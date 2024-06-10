#include "chloros.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void *echo(void *arg);

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  grn_init(true);

  int id = grn_spawn(echo, NULL);

  grn_join(id, NULL);
}

void *echo(void *arg) {
  UNUSED(arg);

  char in[2048];

  int read = grn_read(STDIN_FILENO, in, 1024);

  while (read != 0) {
    in[read - 1] = '\0';
    printf("Thanks for this {%s}\n", in);
    memset(in, 0, 2048);
    read = grn_read(STDIN_FILENO, in, 1024);
  }

  printf("Ok, bye bye!\n");

  return NULL;
}
