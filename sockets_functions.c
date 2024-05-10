
#include "sockets_functions.h"
#include <unistd.h>
#include <string.h>
#include <stdint.h> // Para usar tipos de ancho fijo
#include <arpa/inet.h> // Para ntohl()

int sendMessage(int socket, const char *buffer) {
    int32_t len = strlen(buffer); // Obtiene la longitud del mensaje
    int32_t netLen = htonl(len); // Convierte a formato de red

    // Enviar primero la longitud del mensaje
    if (write(socket, &netLen, sizeof(netLen)) < 0) {
        return -1;
    }

    // Enviar el contenido del mensaje
    int sentBytes = 0;
    while (sentBytes < len) {
        int r = write(socket, buffer + sentBytes, len - sentBytes);
        if (r < 0) return -1; // Error al enviar
        sentBytes += r;
    }

    return 0; // Mensaje enviado exitosamente
}


int recvMessage(int socket, char **buffer) {
    int32_t netLen;
    // Leer la longitud del mensaje
    if (read(socket, &netLen, sizeof(netLen)) <= 0) {
        return -1; // Error al leer o conexión cerrada
    }

    int32_t len = ntohl(netLen); // Convertir la longitud a host byte order

    // Asignar memoria para el mensaje basado en la longitud leída
    *buffer = (char*)malloc(len + 1); // +1 para el carácter nulo al final
    if (*buffer == NULL) {
        return -1; // Error de asignación de memoria
    }

    int readBytes = 0;
    while (readBytes < len) {
        int r = read(socket, *buffer + readBytes, len - readBytes);
        if (r <= 0) {
            free(*buffer); // Liberar memoria en caso de error
            *buffer = NULL; // Evitar usar punteros colgantes
            return -1; // Error al leer o conexión cerrada
        }
        readBytes += r;
    }

    (*buffer)[len] = '\0'; // Asegurar terminación de la cadena
    return 0; // Mensaje recibido exitosamente
}
