#include "chloros.h"
#include "utils.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

void *handle(void *arg) {
  int fd = *((int *)arg);
  free(arg);

  char in[2048];

  int read = grn_read(fd, in, 1024);

  while (read > 0) {
    in[read] = '\n';
    in[read + 1] = '\0';
    printf("Got this {%s}\n", in);
    memset(in, 0, 2048);
    read = grn_read(fd, in, 1024);
  }

  printf("Connection over\n");

  return 0;
}

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  grn_init(true);

  int port = 8080;

  struct sockaddr_in server_ip;
  memset(&server_ip, 0, sizeof(server_ip));

  server_ip.sin_family = AF_INET;
  server_ip.sin_addr.s_addr = htonl(INADDR_ANY);
  server_ip.sin_port = htons(port);

  int listener = socket(AF_INET, SOCK_STREAM, 0);

  int failure = bind(listener, (struct sockaddr *)&server_ip, sizeof(server_ip));

  if (failure) {
    printf("Couldn't bind socket, exiting\n");
    exit(-1);
  }

  listen(listener, 0);

  while (true) {
    int conn = grn_accept(listener, NULL, NULL);
    printf("accepted connection\n");
    int *arg = malloc(sizeof(int));
    *arg = conn;
    grn_spawn(handle, (void *)arg);
  }
}
