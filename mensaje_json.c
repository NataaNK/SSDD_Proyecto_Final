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

    char operation[50];
    sscanf(message, "%s", operation);
    request->op = get_op_code(operation);

    switch (request->op) {
        case 0: // REGISTER
        case 1: // UNREGISTER
        case 7: // DISCONNECT
            sscanf(message, "%*s %s", request->user_name);
            break;
        case 2: // CONNECT
            // Se espera recibir "CONNECT username listen_port"
            sscanf(message, "%*s %s %s", request->user_name, request->listen_port);
            break;
        case 3: // PUBLISH
            sscanf(message, "%*s %s %s %[^\t\n]", request->user_name, request->file_name, request->description);
            break;
        case 4: // DELETE
            sscanf(message, "%*s %s", request->user_name);
            break;
        case 6: // LIST_CONTENT
            sscanf(message, "%*s %s %s", request->user_name, request->remote_user_name);
            break;
        case 5: // LIST_USERS
            sscanf(message, "%*s %s", request->user_name);
            break;
        case 8: // GET_FILE
            sscanf(message, "%*s %s %s %s", request->user_name, request->remote_file_name, request->local_file_name);
            break;
        default:
            sprintf(request->err_msg, "Operación desconocida.");
    }
}

