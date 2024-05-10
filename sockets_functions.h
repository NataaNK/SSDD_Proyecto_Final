#ifndef SOCKETS_FUNCTIONS_H
#define SOCKETS_FUNCTIONS_H

#include <unistd.h>

int sendMessage(int socket, const char *buffer);
int recvMessage(int socket, char **buffer);

#endif // SOCKETS_FUNCTIONS_H