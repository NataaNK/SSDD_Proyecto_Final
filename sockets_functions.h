#ifndef SOCKETS_FUNCTIONS_H
#define SOCKETS_FUNCTIONS_H

#include <unistd.h>

int sendMessage(int socket, char * buffer, int len);
int recvMessage(int socket, char **buffer);

#endif // SOCKETS_FUNCTIONS_H