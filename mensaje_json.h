#ifndef MENSAJE_JSON_H
#define MENSAJE_JSON_H

#include "mensaje.h"

char* serialize_message_to_server(struct peticion request);
void deserialize_message_from_client(const char* message, struct peticion* request);

#endif // MENSAJE_JSON_H