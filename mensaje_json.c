/*
    Autores: Natalia Rodríguez Navarro (100471976)
             Arturo Soto Ruedas (100472007)

    Módulo que implementa la obtención de datos de la petición
    del cliente Pyhton al servidor socket en C.
*/

#include "cJSON.h"
#include "mensaje.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* serialize_message_to_server(struct peticion request) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "op", request.op);
    cJSON_AddStringToObject(root, "user_name", request.user_name);
    cJSON_AddStringToObject(root, "file_name", request.file_name);
    cJSON_AddStringToObject(root, "description", request.description);
    cJSON_AddStringToObject(root, "time", request.time); 
    cJSON_AddStringToObject(root, "err_msg", request.err_msg);

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}


// Función auxiliar para obtener el código de operación basado en el nombre de la operación
int get_op_code(const char* op_name) {
    if (strcmp(op_name, "REGISTER") == 0) return 0;
    if (strcmp(op_name, "UNREGISTER") == 0) return 1;
    if (strcmp(op_name, "CONNECT") == 0) return 2;
    if (strcmp(op_name, "PUBLISH") == 0) return 3;
    if (strcmp(op_name, "DELETE") == 0) return 4;
    if (strcmp(op_name, "LIST_USERS") == 0) return 5;
    if (strcmp(op_name, "LIST_CONTENT") == 0) return 6;
    if (strcmp(op_name, "DISCONNECT") == 0) return 7;
    if (strcmp(op_name, "GET_FILE") == 0) return 8;
    return -1;  // Código desconocido
}


void deserialize_message_from_client(const char* message, struct peticion* request) {
    memset(request, 0, sizeof(struct peticion));

    char operation[50], date[11], time[9];  // Formato de fecha MM/DD/YYYY y HH:MM:SS
    char dateTime[21]; // Buffer para almacenar fecha y hora concatenadas
    int scanCount = sscanf(message, "%s %10s %8s", operation, date, time);  // Extraer operación, fecha y hora

    if (scanCount != 3) {
        fprintf(stderr, "Error parsing the operation, date, or time.\n");
        return; // Manejo de error si no se extraen los tres componentes correctamente
    }

    snprintf(dateTime, sizeof(dateTime), "%s %s", date, time); // Concatenar fecha y hora de forma segura
    strcpy(request->time, dateTime); // Copiar al campo time del request

    request->op = get_op_code(operation);  // Obtener código de operación

    // Avanzar el puntero del mensaje más allá de la operación, fecha y hora
    const char* next = strstr(message, time) + strlen(time);
    if (*next == ' ') next++; // Asegurarse de que se salta el espacio después del tiempo

    switch (request->op) {
        case 0: // REGISTER
        case 1: // UNREGISTER
        case 7: // DISCONNECT
            sscanf(next, "%s", request->user_name);  // Extraer user_name
            break;
        case 2: // CONNECT
            sscanf(next, "%s %s", request->user_name, request->listen_port);
            break;
        case 3: // PUBLISH
            sscanf(next, "%s %s %[^\t\n]", request->user_name, request->file_name, request->description);
            break;
        case 4: // DELETE
            sscanf(next, "%s %s", request->user_name, request->file_name);
            break;
        case 5: // LIST_USERS
            sscanf(next, "%s", request->user_name);
            break;
        case 6: // LIST_CONTENT
            sscanf(next, "%s %s", request->user_name, request->remote_user_name);
            break;
        case 8: // GET_FILE
            sscanf(next, "%s %s %s %s", request->user_name, request->remote_user_name, request->remote_file_name, request->local_file_name);
            break;
        default:
            sprintf(request->err_msg, "Operación desconocida.");
            break;
    }
}
