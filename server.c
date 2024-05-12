/*
    Autores: Natalia Rodríguez Navarro (100471976)
             Arturo Soto Ruedas (100472007)

    Módulo que implementa el servidor socket que realiza las operaciones
	y se encarga de la lógica de las peticiones del cliente socket en Python.
	También se comunica con el servidor RPC para mandarle una operación.
*/

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include "cJSON/cJSON.h"
#include "mensaje.h"
#include <stddef.h> 
#include <errno.h> 
#include <signal.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <net/if.h>
#include "RPC_Print_client.h"
#include "sockets_functions.h"
#include "mensaje_json.h"


/* PROTOTIPOS */
cJSON* read_json(char *file_path);
int write_json(char *file_path, char *str);
void handle_signal(int sig);

/* Variables globales */
int sd;
int sc;

/* Mutex y variables condicionales para proteger la copia del mensaje */
pthread_mutex_t mutex_mensaje;
int mensaje_no_copiado = true;
pthread_cond_t cond_mensaje;

/* Mutex para proteger el acceso al archivo data.json */
pthread_mutex_t mutex_archivo;


/* Argumentos de tratar mensaje*/
struct args_tratar_msg  {
    struct peticion mess;
    struct sockaddr_in client_addr;
    int sc;
};

void *tratar_mensaje(void *args_trat_msg){
	struct peticion mensaje;	/* mensaje local */
	struct sockaddr_in client_addr; /* addr del cliente local*/
	int resultado;	/* resultado de la operación */
	int sc;						/* socket de conexión con el cliente */
	char client_ip[INET_ADDRSTRLEN]; /* para almacenar la dirección IP del cliente en formato legible */

	/* el thread copia el mensaje a un mensaje local */
	pthread_mutex_lock(&mutex_mensaje);

	mensaje = ((struct args_tratar_msg *)args_trat_msg)->mess;
	memcpy(&client_addr, &((struct args_tratar_msg *)args_trat_msg)->client_addr, sizeof(struct sockaddr_in));
	memcpy(&sc, &((struct args_tratar_msg *)args_trat_msg)->sc, sizeof(int));

    /* Convertir la dirección IP del cliente de binario a texto */
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

	/* ya se puede despertar al servidor*/
	mensaje_no_copiado = false;

	pthread_cond_signal(&cond_mensaje);

	pthread_mutex_unlock(&mutex_mensaje);

	resultado = 0;

	/* ejecutar la petición del cliente y preparar respuesta */
	/* REGISTER */
	if (mensaje.op == 0){
		printf("REGISTER FROM %s\n", mensaje.user_name);

		// LLAMAR A SERVIDOR RPC PARA QUE IMPRIMA LA PETICIÓN
		strcpy(mensaje.file_name, "");
		char op_name[9] = "REGISTER\0";
		print_user_op(mensaje.user_name, op_name, mensaje.file_name, mensaje.time);

		// Leer cotenido base de datos y obtener json
		cJSON *json = read_json("data.json");
		if (json == NULL){
			resultado = 2;
		}
		else {
			// Crear entrada en el json, con la clave como user_name

			// Si encuentra un item usando esa clave, la clave ya existe -> error
			cJSON *item = cJSON_GetObjectItemCaseSensitive(json, mensaje.user_name);
			if (item != NULL){
				resultado = 1;
			}
			else {
				// Array vacío
				cJSON_AddArrayToObject(json,  mensaje.user_name);

				// Convertir cJSON object a JSON string 
				char *json_str = cJSON_Print(json);
				
				// Escribir JSON string al archivo
				if (write_json("data.json", json_str) < 0){
					resultado = 1;
				}

				// Liberar JSON string y cJSON object 
				cJSON_free(json_str); 
				cJSON_Delete(json);
			}
		}

	/* UNREGISTER */
    } else if (mensaje.op == 1){
		printf("UNREGISTER FROM %s\n", mensaje.user_name);

		// LLAMAR A SERVIDOR RPC PARA QUE IMPRIMA LA PETICIÓN
		strcpy(mensaje.file_name, "");
		char op_name[11] = "UNREGISTER\0";
		print_user_op(mensaje.user_name, op_name, mensaje.file_name, mensaje.time);

		// Leer cotenido base de datos y obtener json
		cJSON *json = read_json("data.json");
		if (json == NULL){
			resultado = 2;
		}
		else {
			// Eliminar entrada
			cJSON *item = cJSON_GetObjectItemCaseSensitive(json, mensaje.user_name);
			if (item == NULL){
				resultado = 1; // No existe la entrada
			}
			else {
				cJSON_DeleteItemFromObject(json, mensaje.user_name);
			
				// Covertir cJSON object a JSON string 
				char *json_str = cJSON_Print(json); 
			
				// Escribir JSON string al archivo
				if (write_json("data.json", json_str) < 0){
					resultado = 2;
				}

				// Liberar el JSON string y cJSON object 
				cJSON_free(json_str); 
				cJSON_Delete(json);
			}
		}

	/* CONNECT */
    } else if (mensaje.op == 2){
		printf("CONNECT %s\n", mensaje.user_name);

		// LLAMAR A SERVIDOR RPC PARA QUE IMPRIMA LA PETICIÓN
		strcpy(mensaje.file_name, "");
		char op_name[8] = "CONNECT\0";
		print_user_op(mensaje.user_name, op_name, mensaje.file_name, mensaje.time);

		// Leer cotenido base de datos y obtener json
		cJSON *json = read_json("users_connected.json");
		if (json == NULL){
			resultado = 3;
		}
		else{
			cJSON *json_data = read_json("data.json");
			if (json == NULL){
				resultado = 3;
			} 
			else{
				// Si no encuentra un item usando esa clave, el usuario no existe
				cJSON *item_exists = cJSON_GetObjectItemCaseSensitive(json_data, mensaje.user_name);
				if (item_exists == NULL){
					resultado = 1;
				}
				else{
					// Si encuentra un item usando esa clave, el usuario ya está conectado
					cJSON *item = cJSON_GetObjectItemCaseSensitive(json, mensaje.user_name);
					if (item != NULL){
						resultado = 2;
					} 
					else {
						// Array [str(IP_cliente), str(Puerto_cliente)]
						cJSON *user_info = cJSON_CreateArray();
						cJSON_AddItemToArray(user_info, cJSON_CreateString(client_ip)); // IP del cliente
						cJSON_AddItemToArray(user_info, cJSON_CreateString(mensaje.listen_port)); // Puerto de escucha del cliente

						cJSON_AddItemToObject(json, mensaje.user_name, user_info);

						char *json_str = cJSON_Print(json);
						if (write_json("users_connected.json", json_str) < 0) {
							resultado = 3; // Error al escribir en el archivo
						}
						cJSON_free(json_str);
					}
					cJSON_Delete(json);
					cJSON_Delete(json_data);
				}
			}
		}
		

	/* PUBLISH */
    } else if (mensaje.op == 3){
		printf("PUBLISH FROM %s\n", mensaje.user_name);

		// LLAMAR A SERVIDOR RPC PARA QUE IMPRIMA LA PETICIÓN
		char op_name[8] = "PUBLISH\0";
		print_user_op(mensaje.user_name, op_name, mensaje.file_name, mensaje.time);

		// Leer cotenido base de datos y obtener json
		cJSON *json = read_json("data.json");
		if (json == NULL){
			resultado = 4;
		}
		else {
			cJSON *json_conn = read_json("users_connected.json");
			if (json_conn == NULL){
				resultado = 4;
			}
			else{
				// Si el usuario no está conectado
				cJSON *item_conn = cJSON_GetObjectItemCaseSensitive(json_conn, mensaje.user_name);
				if (item_conn == NULL){
					resultado = 2;
				}
				else {
					// Si no existe el usuario
					cJSON *item = cJSON_GetObjectItemCaseSensitive(json, mensaje.user_name);
					if (item == NULL){
						resultado = 1;
					}
					else{
						// Comprobamos si ya ha publicado ese contenido
						cJSON *array = cJSON_GetObjectItemCaseSensitive(json, mensaje.user_name);
						int array_size = cJSON_GetArraySize(array);

						char** strings = malloc(array_size * sizeof(char*));
						if (strings == NULL) {
							fprintf(stderr, "No se pudo asignar memoria para el arreglo de strings\n");
							cJSON_Delete(json);
							resultado = 4;
						}

						for (int i = 0; i < array_size; i++) {
							cJSON *item = cJSON_GetArrayItem(array, i);
							strings[i] = cJSON_GetStringValue(item);

							char *firstWord = strtok(strings[i], " ");  // Extraer la primera palabra del string.
							if (firstWord != NULL && strcmp(firstWord, mensaje.file_name) == 0) {
								resultado = 3;
							}
						}

						// Si no está publicado aún añadirlo
						if (resultado != 3){
							char file_and_description[512];
							strcpy(file_and_description, mensaje.file_name);
							strcat(file_and_description, " ");
							strcat(file_and_description, mensaje.description);
							cJSON *new_item = cJSON_CreateString(file_and_description);
    						cJSON_AddItemToArray(array, new_item);
							// Convertir cJSON object a JSON string 
							char *json_str = cJSON_Print(json);
							
							// Escribir JSON string al archivo
							if (write_json("data.json", json_str) < 0){
								resultado = 4;
							}

							cJSON_free(json_str); 
						}
					}
					
				}
				cJSON_Delete(json_conn);
			}
			cJSON_Delete(json);
		}
	

	/* DELETE */
    } else if (mensaje.op == 4){
		printf("DELETE FROM %s\n", mensaje.user_name);

		// LLAMAR A SERVIDOR RPC PARA QUE IMPRIMA LA PETICIÓN
		char op_name[7] = "DELETE\0";
		print_user_op(mensaje.user_name, op_name, mensaje.file_name, mensaje.time);

		// Leer cotenido base de datos y obtener json
		cJSON *json = read_json("data.json");
		if (json == NULL){
			resultado = 4;
		}
		else {
			cJSON *json_conn = read_json("users_connected.json");
			if (json_conn == NULL){
				resultado = 4;
			}
			else{
				// Si el usuario no está conectado
				cJSON *item_conn = cJSON_GetObjectItemCaseSensitive(json_conn, mensaje.user_name);
				if (item_conn == NULL){
					resultado = 2;
				}
				else {
					// Si no existe el usuario
					cJSON *item = cJSON_GetObjectItemCaseSensitive(json, mensaje.user_name);
					if (item == NULL){
						resultado = 1;
					}
					else{
						// Comprobamos si existe ese contenido
						cJSON *array = cJSON_GetObjectItemCaseSensitive(json, mensaje.user_name);
						int array_size = cJSON_GetArraySize(array);

						char** strings = malloc(array_size * sizeof(char*));
						if (strings == NULL) {
							fprintf(stderr, "No se pudo asignar memoria para el arreglo de strings\n");
							cJSON_Delete(json);
							resultado = 4;
						}

						int indice_publicado = -1;
						for (int i = 0; i < array_size; i++) {
							cJSON *item = cJSON_GetArrayItem(array, i);
							strings[i] = cJSON_GetStringValue(item);  // Hacer una copia para manipulación segura.

							// Usar strtok para obtener la primera palabra
							char *firstWord = strtok(strings[i], " ");  // Extraer la primera palabra del string.
							if (firstWord != NULL && strcmp(firstWord, mensaje.file_name) == 0) {
								indice_publicado = i;
								break;  // Si se encuentra el archivo, detener la búsqueda.
							}
						}

						if (indice_publicado == -1){
							resultado = 3;
						}
						else{
							cJSON_DeleteItemFromArray(array, indice_publicado);

							// Covertir cJSON object a JSON string 
							char *json_str = cJSON_Print(json); 
						
							// Escribir JSON string al archivo
							if (write_json("data.json", json_str) < 0){
								resultado = 4;
							}

							// Liberar el JSON string y cJSON object 
							cJSON_free(json_str); 
						}

					}
					
				}
				cJSON_Delete(json_conn);
			}
			cJSON_Delete(json);
		}
    } 

	/* LIST_USERS */
    else if (mensaje.op == 5){
		printf("LIST_USERS FROM %s\n", mensaje.user_name);

		// LLAMAR A SERVIDOR RPC PARA QUE IMPRIMA LA PETICIÓN
		strcpy(mensaje.file_name, "");
		char op_name[11] = "LIST_USERS\0";
		print_user_op(mensaje.user_name, op_name, mensaje.file_name, mensaje.time);

		// Leer cotenido base de datos y obtener json
		cJSON *json = read_json("data.json");
		if (json == NULL){
			resultado = 3;
		}
		else {
			cJSON *json_conn = read_json("users_connected.json");
			if (json_conn == NULL){
				resultado = 3;
			}
			else{
				// Si el usuario no está conectado
				cJSON *item_conn = cJSON_GetObjectItemCaseSensitive(json_conn, mensaje.user_name);
				if (item_conn == NULL){
					resultado = 2;
				}
				else {
					// Si no existe el usuario
					cJSON *item = cJSON_GetObjectItemCaseSensitive(json, mensaje.user_name);
					if (item == NULL){
						resultado = 1;
					}
					else{
						// Enviar código 0
						int err;
						resultado = htonl(resultado);
						err = sendMessage(sc, (char *)&resultado, sizeof(int32_t)); 
						if (err == -1) {
							printf("Error en envío\n");
							close(sc);
						}

						// Calcular usuarios a enviar
						// Contar claves
						int key_count = 0;
						cJSON *current_element = NULL;
						cJSON_ArrayForEach(current_element, json_conn) {
							key_count++;  // Incrementar por cada clave encontrada
						}

						// Enviar al cliente cuantos usuarios hay 
						key_count = htonl(key_count);
						err = sendMessage(sc, (char *)&key_count, sizeof(int32_t));  
						if (err == -1) {
							printf("Error en envío\n");
							close(sc);
						}

						// Enviar info usuarios, línea a línea
						cJSON *current_user = NULL;
						cJSON_ArrayForEach(current_user, json_conn) {
							if (!cJSON_IsArray(current_user)) {
								fprintf(stderr, "Error: expected an array for key '%s'\n", current_user->string);
								continue;
							}

							int array_size = cJSON_GetArraySize(current_user);
							char buffer[1024] = {0}; 
							int buffer_length = snprintf(buffer, sizeof(buffer), "%s", current_user->string);

							for (int i = 0; i < array_size; i++) {
								cJSON *item = cJSON_GetArrayItem(current_user, i);
								if (!cJSON_IsString(item)) {
									fprintf(stderr, "Error: array item is not a string\n");
									continue;
								}

								buffer_length += snprintf(buffer + buffer_length, sizeof(buffer) - buffer_length, " %s", item->valuestring);
							}

							snprintf(buffer + buffer_length, sizeof(buffer) - buffer_length, "\n");

							if (sendMessage(sc, buffer, strlen(buffer)) != 0) {
								fprintf(stderr, "Failed to send message\n");
								resultado = 3; 
							}
						}

						}
					}
					
				}
				cJSON_Delete(json_conn);
			}
			cJSON_Delete(json);
	}
	/* LIST_CONTENT */
    else if (mensaje.op == 6){
		printf("LIST_CONTENT FROM %s\n", mensaje.user_name);

		// LLAMAR A SERVIDOR RPC PARA QUE IMPRIMA LA PETICIÓN
		strcpy(mensaje.file_name, "");
		char op_name[13] = "LIST_CONTENT\0";
		print_user_op(mensaje.user_name, op_name, mensaje.file_name, mensaje.time);

		// Leer cotenido base de datos y obtener json
		cJSON *json = read_json("data.json");
		if (json == NULL){
			resultado = 3;
		}
		else {
			cJSON *json_conn = read_json("users_connected.json");
			if (json_conn == NULL){
				resultado = 3;
			}
			else{
				// Si el usuario no está conectado
				cJSON *item_conn = cJSON_GetObjectItemCaseSensitive(json_conn, mensaje.user_name);
				if (item_conn == NULL){
					resultado = 2;
				}
				else {
					// Si no existe el usuario
					cJSON *item = cJSON_GetObjectItemCaseSensitive(json, mensaje.user_name);
					cJSON *item_remote = cJSON_GetObjectItemCaseSensitive(json, mensaje.remote_user_name);
					if (item == NULL){
						resultado = 1;
					}
					// Si no existe el usuario remoto
					else if (item_remote == NULL) {
						resultado = 3;
					}
					else{
					
						// Enviar código 0
						int err;
						resultado = htonl(resultado);
						err = sendMessage(sc, (char *)&resultado, sizeof(int32_t)); 
						if (err == -1) {
							printf("Error en envío\n");
							close(sc);
						}

						// Calcular publicaciones a enviar
						int content_count = cJSON_GetArraySize(item_remote);
		

						// Enviar al cliente cuantas publicaciones hay 
						content_count = htonl(content_count);
						err = sendMessage(sc, (char *)&content_count, sizeof(int32_t));  
						if (err == -1) {
							printf("Error en envío\n");
							close(sc);
						}

						// Enviar info publicaciones, línea a línea
						int i;
						for (i = 0; i < content_count; i++) {
							cJSON *content_item = cJSON_GetArrayItem(item_remote, i);
							if (content_item != NULL) {
								char *content_str = cJSON_PrintUnformatted(content_item);
								if (content_str != NULL) {
									// Enviar cada publicación como una línea
									sendMessage(sc, content_str, strlen(content_str));
									sendMessage(sc, "\n", 1);  // Enviar nueva línea para separar las publicaciones
									free(content_str);
								}
							}
						}


						}
					}
					
				}
				cJSON_Delete(json_conn);
			}
			cJSON_Delete(json);
    } 
	/* DISCONNECT */
    else if (mensaje.op == 7){
		printf("DISCONNECT FROM %s\n", mensaje.user_name);
	
		// LLAMAR A SERVIDOR RPC PARA QUE IMPRIMA LA PETICIÓN
		strcpy(mensaje.file_name, "");
		char op_name[11] = "DISCONNECT\0";
		print_user_op(mensaje.user_name, op_name, mensaje.file_name, mensaje.time);

		// Leer cotenido base de datos y obtener json
		cJSON *json = read_json("data.json");
		cJSON *json_conn = read_json("users_connected.json");
		if (json == NULL){
			resultado = 3;
		}
		else if(json_conn == NULL){
			resultado = 3;
		}
		else {
			// No existe el usuario
			cJSON *item = cJSON_GetObjectItemCaseSensitive(json, mensaje.user_name);
			cJSON *item_conn = cJSON_GetObjectItemCaseSensitive(json_conn, mensaje.user_name);
			if (item == NULL){
				resultado = 1; 
			}
			// El usuario no está conectado
			else if (item_conn == NULL){
				resultado = 2;
			}
			else {
				cJSON_DeleteItemFromObject(json_conn, mensaje.user_name);
			
				// Covertir cJSON object a JSON string 
				char *json_str = cJSON_Print(json_conn); 
			
				// Escribir JSON string al archivo
				if (write_json("users_connected.json", json_str) < 0){
					resultado = 2;
				}

				// Liberar el JSON string y cJSON object 
				cJSON_free(json_str); 
				cJSON_Delete(json);
				cJSON_Delete(json_conn);
			}
		}
	}
	/* GET_FILE*/
	else if (mensaje.op == 8){
		printf("GET_FILE FROM %s\n", mensaje.user_name);

		// Leer cotenido base de datos y obtener json
		cJSON *json = read_json("data.json");
		if (json == NULL){
			resultado = 3;
		}
		else {
			cJSON *json_conn = read_json("users_connected.json");
			if (json_conn == NULL){
				resultado = 3;
			}
			else{
				// Si el usuario no está conectado
				cJSON *item_conn = cJSON_GetObjectItemCaseSensitive(json_conn, mensaje.user_name);
				cJSON *item_rem = cJSON_GetObjectItemCaseSensitive(json, mensaje.remote_user_name);
				cJSON *item_conn_rem = cJSON_GetObjectItemCaseSensitive(json_conn, mensaje.remote_user_name);
				if (item_conn == NULL){
					resultado = 2;
				}
				// Si el usuario remoto no existe
				else if (item_rem == NULL){
					resultado = 4;
				}
				// Si el usuario remoto no está conectado
				else if(item_conn_rem == NULL){
					resultado = 5;
				}
				else {
					// Si no existe el usuario
					cJSON *item = cJSON_GetObjectItemCaseSensitive(json, mensaje.user_name);
					if (item == NULL){
						resultado = 1;
					}
					else{
						 // Verificar si el archivo está publicado
						cJSON *files = cJSON_GetObjectItemCaseSensitive(json, mensaje.remote_user_name);
						char *file_list = cJSON_Print(files);
						if (!strstr(file_list, mensaje.remote_file_name)) {
							resultado = 6;  // Archivo no está publicado
						} else {
							// Enviar código 0
							int err;
							resultado = htonl(resultado);
							err = sendMessage(sc, (char *)&resultado, sizeof(int32_t)); 
							if (err == -1) {
								printf("Error en envío\n");
								close(sc);
							}

							// El archivo está publicado, proceder a enviar ip y puerto
							cJSON *remote_user_info = cJSON_GetArrayItem(item_conn_rem, 0);
							cJSON *remote_user_port = cJSON_GetArrayItem(item_conn_rem, 1);
							char *ip = cJSON_GetStringValue(remote_user_info);
							char *port = cJSON_GetStringValue(remote_user_port);
							char response[1024];
							snprintf(response, sizeof(response), "%s %s", ip, port); // IP y puerto
							send(sc, response, strlen(response), 0); // Enviar datos al cliente solicitante
						}
						free(file_list);
						}
					}
				}
				cJSON_Delete(json_conn);
			}
			cJSON_Delete(json);
	}

	int err;
	resultado = htonl(resultado);
	err = sendMessage(sc, (char *)&resultado, sizeof(int32_t));  // envía el resultado
	if (err == -1) {
		printf("Error en envío\n");
		close(sc);
	}

	pthread_exit(NULL);
	return(NULL);
}



cJSON* read_json(char *file_path){
	
	pthread_mutex_lock(&mutex_archivo); // Adquirir el mutex antes de leer

	FILE *fp = fopen(file_path, "r"); 
	if (fp == NULL) { 
		perror("Error: Unable to open the file.\n"); 
		return NULL; 
	} 

	// Leer contenido de la base de datos, tamaño dinámico
	char *buffer = NULL;
	size_t file_size = 0;
	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	buffer = (char *)malloc(file_size + 1);
	fread(buffer, 1, file_size, fp);
	buffer[file_size] = '\0';
	fclose(fp); 

	pthread_mutex_unlock(&mutex_archivo); // Liberar el mutex después de leer

	// Parse los datos del json
	cJSON *json = cJSON_Parse(buffer); 
	if (json == NULL) { 
		const char *error_ptr = cJSON_GetErrorPtr(); 
		if (error_ptr != NULL) { 
			printf("Error: %s\n", error_ptr); 
		} 
		cJSON_Delete(json); 
		return NULL; 
	} 
	return json;
}

int write_json(char *file_path, char *str){

	pthread_mutex_lock(&mutex_archivo); // Adquirir el mutex antes de escribir

	FILE *fp = fopen(file_path, "w"); 
	if (fp == NULL) { 
		perror("Error: Unable to open the file.\n"); 
		return -1; 
	} 

	fputs(str, fp);
	fclose(fp); 

	pthread_mutex_unlock(&mutex_archivo); // Liberar el mutex después de escribir

	return 0;
}


int main(int argc, char **argv) {   
	struct sockaddr_in server_addr, client_addr;
	socklen_t size;
	int val;
	struct peticion pet;
	int err; 
    pthread_attr_t t_attr;	
    pthread_t thid;
	struct args_tratar_msg args_trat_msg;
    char ip_display[INET_ADDRSTRLEN] = "0.0.0.0";

	if (argc != 3){
		printf("Usage: ./ server -p < port >\n");
		return -1;
	}

	if(strcmp(argv[1], "-p")!=0){
		printf("Usage: ./ server -p < port >\n");
		return -1;
	}

	if ((sd =  socket(AF_INET, SOCK_STREAM, 0))<0){
		printf ("SERVER: Error en el socket");
		return (0);
	}

	val = 1;
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *) &val, sizeof(int));
  
    struct ifaddrs *addresses;
    if (getifaddrs(&addresses) == -1) {
        perror("Failed to get network interfaces");
        return -1;
    }

    struct ifaddrs *address = addresses;
    while (address) {
        int family = address->ifa_addr->sa_family;
        if (family == AF_INET) {
            struct sockaddr_in *addr = (struct sockaddr_in *)address->ifa_addr;
            char *ip = inet_ntoa(addr->sin_addr);
            if (strcmp(ip, "127.0.0.1") != 0) {  // Check not loopback IP
                strcpy(ip_display, ip);
                break;
            }
        }
        address = address->ifa_next;
    }

    freeifaddrs(addresses);

	bzero((char *)&server_addr, sizeof(server_addr));
	server_addr.sin_family      = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port        = htons(atoi(argv[2]));

	err = bind(sd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
	if (err == -1) {
		printf("Error en bind\n");
		return -1;
	}

	pthread_mutex_init(&mutex_mensaje, NULL);
	pthread_cond_init(&cond_mensaje, NULL);
	pthread_mutex_init(&mutex_archivo, NULL);
	pthread_attr_init(&t_attr);

	// atributos de los threads, threads independientes
	pthread_attr_setdetachstate(&t_attr, PTHREAD_CREATE_DETACHED);


    err = listen(sd, SOMAXCONN);
	if (err == -1) {
		printf("Error en listen\n");
		return -1;
	}
	size = sizeof(client_addr);
	// Si se produce Ctrl+C o error, cerramos socket
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    printf("s> init server <%s>:<%d>\n", ip_display, ntohs(server_addr.sin_port));
	while (1){
		printf("s> ");
        fflush(stdout);
		sc = accept(sd, (struct sockaddr *)&client_addr, (socklen_t *)&size);

		if (sc == -1) {
			printf("Error en accept\n");
			return -1;
		}

		// Recibe el mensaje directamente en el buffer 'pet_json'
		char *pet_json = NULL;
		err = recvMessage(sc, &pet_json);
		if (err == -1) {
			printf("Error en recepción\n");
			close(sc); 
			free(pet_json); 
			continue; 
		}


		// Deserializa el mensaje JSON en la estructura 'pet'
		deserialize_message_from_client(pet_json, &pet); // Pasa 'pet_json' directamente
		free(pet_json); 


		memcpy(&args_trat_msg.mess, &pet, sizeof(struct peticion));
		memcpy(&args_trat_msg.client_addr, &client_addr, sizeof(struct sockaddr_in));
		args_trat_msg.sc = sc;

		if (pthread_create(&thid, &t_attr, tratar_mensaje, (void *)&args_trat_msg) == 0) {
            // se espera a que el thread copie el mensaje 
            pthread_mutex_lock(&mutex_mensaje);
            while (mensaje_no_copiado){
                pthread_cond_wait(&cond_mensaje, &mutex_mensaje);
            }
            mensaje_no_copiado = true;
            pthread_mutex_unlock(&mutex_mensaje);
        }   
	}
	close(sd);
	close(sc);
    return(0);
}

/* Si se produce Ctrl+C y error: cerramos socket*/
void handle_signal(int sig) {
    if (sig == SIGINT) {
		close(sd);
		close(sc);
    } else if (sig == SIGTERM) {
		close(sd);
		close(sc);
    }

    // Salir del programa después de recibir la señal
    exit(0);
}