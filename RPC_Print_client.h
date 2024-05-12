#ifndef RPC_PRINT_CLIENT_H
#define RPC_PRINT_CLIENT_H


/**
 * @brief Este servicio permite imprimir por pantalla la operación en el servidor RPC.
 * 
 * 
 * @param host Ip del servidor RPC.
 * @param user_name nombre del usuario que hace la operación [256].
 * @param op_name nombre de operacion que realiza el usuario [15].
 * @param file_name nombre del archivo relativo a la operación, si no es necesario, pasar cadena vacía "" [256].
 * @param time string con la fecha en formato "%d/%m/%Y %h:%m:%S" [20].
 * @retval void.
 */
void print_user_op(char *user_name, char *op_name, char *file_name, char *time);

#endif