
#include "sockets_functions.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h> // Para usar tipos de ancho fijo
#include <arpa/inet.h> // Para ntohl()

int sendMessage(int socket, char * buffer, int len)
{
	int r;
	int l = len;
		

	do {	
		r = write(socket, buffer, l);
		l = l -r;
		buffer = buffer + r;
	} while ((l>0) && (r>=0));
	
	if (r < 0)
		return (-1);   /* fail */
	else
		return(0);	/* full length has been sent */
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
