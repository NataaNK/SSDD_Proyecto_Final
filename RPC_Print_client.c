/*
    Autores: Natalia Rodríguez Navarro (100471976)
             Arturo Soto Ruedas (100472007)

    Módulo que implementa la biblioteca de  cliente RPC
    para poder acceder al servicio remoto del servidor RPC.
*/


#include "RPC_Print.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_user_op(char *user_name, char *op_name, char *file_name, char *time);

CLIENT* communication_with_server() {
    CLIENT *clnt = clnt_create("localhost", PRINT_PROG, PRINT_VERS, "tcp");
    if (clnt == NULL) {
        clnt_pcreateerror("localhost");
        return NULL;
    }

    return clnt;
}

void print_user_op(char *user_name, char *op_name, char *file_name, char *time) {

    CLIENT *clnt = communication_with_server();
    if (clnt == NULL) {
        printf("Error al comunicarse con el servidor\n");
        exit(1);
    }

    enum clnt_stat retval;
    PrintArgs args;
	void *res = NULL; 

    // Obtener datos de los argumentos
    args.User_name = malloc(strlen(user_name)+1);
    args.Op_name = malloc(strlen(op_name)+1);
    args.File_name  = malloc(strlen(file_name)+1);
    args.Time = malloc(strlen(time)+1);
    strcpy(args.User_name, user_name);
    strcpy(args.Op_name, op_name);
    strcpy(args.File_name, file_name);
    strcpy(args.Time, time);


    retval = print_user_op_1(args, res, clnt);
    if (retval != RPC_SUCCESS) {
        clnt_perror(clnt, "call failed");
        clnt_destroy(clnt);
        free(args.User_name);
        free(args.Op_name);
        free(args.File_name);
        free(args.Time);
        exit(1);
    }

    free(args.User_name);
    free(args.Op_name);
    free(args.File_name);
    free(args.Time);
    clnt_destroy(clnt);
}