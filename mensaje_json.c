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

char* serialize_message_to_client(struct peticion request) {
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

void deserialize_message_from_server(const char* message, struct peticion* request) {
    cJSON *root = cJSON_Parse(message);
    if (root == NULL) {
        return;  // Error handling for failed parse
    }

    cJSON *op = cJSON_GetObjectItemCaseSensitive(root, "op");
    if (cJSON_IsNumber(op)) {
        request->op = op->valueint;
    }

    cJSON *user_name = cJSON_GetObjectItemCaseSensitive(root, "user_name");
    if (cJSON_IsString(user_name) && (user_name->valuestring != NULL)) {
        strcpy(request->user_name, user_name->valuestring);
    }

    cJSON *file_name = cJSON_GetObjectItemCaseSensitive(root, "file_name");
    if (cJSON_IsString(file_name) && (file_name->valuestring != NULL)) {
        strcpy(request->file_name, file_name->valuestring);
    }

    cJSON *description = cJSON_GetObjectItemCaseSensitive(root, "description");
    if (cJSON_IsString(description) && (description->valuestring != NULL)) {
        strcpy(request->description, description->valuestring);
    }

    cJSON *err_msg = cJSON_GetObjectItemCaseSensitive(root, "err_msg");
    if (cJSON_IsString(err_msg) && (err_msg->valuestring != NULL)) {
        strcpy(request->err_msg, err_msg->valuestring);
    }

    cJSON_Delete(root);
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
    // Limpiar estructura peticion
    memset(request, 0, sizeof(struct peticion));

    // Asumiendo que el mensaje es una cadena simple tipo "OPERACION argumentos"
    char operation[50];
    sscanf(message, "%s", operation);  // Leer el primer token como la operación
    request->op = get_op_code(operation);

    // Analizar el mensaje completo basado en la operación
    switch (request->op) {
        case 0: // REGISTER
        case 1: // UNREGISTER
        case 2: // CONNECT
        case 7: // DISCONNECT
            sscanf(message, "%*s %s", request->user_name);  // Leer el segundo token como user_name
            break;
        case 3: // PUBLISH
            sscanf(message, "%*s %s %s %[^\t\n]", request->user_name, request->file_name, request->description);
            break;
        case 4: // DELETE
        case 6: // LIST_CONTENT
            sscanf(message, "%*s %s %s", request->user_name, request->file_name);
            break;
        case 5: // LIST_USERS
            sscanf(message, "%*s %s", request->user_name);
            break;
        case 8: // GET_FILE
            sscanf(message, "%*s %s %s %s", request->user_name, request->remote_file_name, request->loca_file_name);
            break;
        default:
            sprintf(request->err_msg, "Operación desconocida.");
    }
}
