#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "utils.h"

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  int port = 8080;

  struct sockaddr_in server_ip;
  memset(&server_ip, 0, sizeof(server_ip));
  server_ip.sin_family = AF_INET;
  server_ip.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_ip.sin_port = htons(port);

  int client = socket(AF_INET, SOCK_STREAM, 0);

  int failure =
      connect(client, (struct sockaddr *)&server_ip, sizeof(server_ip));

  if (failure) {
    printf("Couldn't connect to server, exiting\n");
    exit(-1);
  }

  char in[2048];

  int read_bytes = read(STDIN_FILENO, in, 1024);

  while (read_bytes != 0) {
    in[read_bytes - 1] = '\0';
    printf("got this{%s}\n", in);
    int bytes_written = write(client, in, read_bytes);
    (void)bytes_written;
    memset(in, 0, 2048);
    read_bytes = read(STDIN_FILENO, in, 1024);
  }

  printf("Ok, bye bye!\n");
}
