/*
    Autores: Natalia Rodríguez Navarro (100471976)
             Arturo Soto Ruedas (100472007)

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
#include "sockets_functions.h"
#include "mensaje_json.h"


/* PROTOTIPOS */
cJSON* read_json(char *file_path);
int write_json(char *file_path, char *str);
void handle_signal(int sig);

/* Variables globales */
int sd;

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

int tratar_mensaje(void *args_trat_msg){
	struct peticion mensaje;	/* mensaje local */
	struct sockaddr_in client_addr; /* addr del cliente local*/
	struct peticion resultado;	/* resultado de la operación */
	int sc;						/* socket de conexión con el cliente */

	/* el thread copia el mensaje a un mensaje local */
	pthread_mutex_lock(&mutex_mensaje);

	mensaje = ((struct args_tratar_msg *)args_trat_msg)->mess;
	memcpy(&client_addr, &((struct args_tratar_msg *)args_trat_msg)->client_addr, sizeof(struct sockaddr_in));
	memcpy(&sc, &((struct args_tratar_msg *)args_trat_msg)->sc, sizeof(int));

	/* ya se puede despertar al servidor*/
	mensaje_no_copiado = false;

	pthread_cond_signal(&cond_mensaje);

	pthread_mutex_unlock(&mutex_mensaje);

	strcpy(resultado.err_msg, "");

	/* ejecutar la petición del cliente y preparar respuesta */
	/* REGISTER */
	if (mensaje.op == 0){
		printf("REGISTER\n");

	/* UNREGISTER */
    } else if (mensaje.op == 1){
		printf("UNREGISTER\n");

	/* CONNECT */
    } else if (mensaje.op == 2){
		printf("CONNECT\n");


	/* PUBLISH */
    } else if (mensaje.op == 3){
		printf("PUBLISH\n");


	/* DELETE */
    } else if (mensaje.op == 4){
		printf("DELETE\n");

    } 

	/* LIST_USERS */
    else if (mensaje.op == 5){
		printf("LIST_USERS\n");

    } 
	/* LIST_CONTENT */
    else if (mensaje.op == 6){
		printf("LIST_CONTENT\n");

    } 
	/* DISCONNECT */
    else if (mensaje.op == 7){
		printf("DISCONNECT\n");
	}
	/* GET_FILE*/
	else if (mensaje.op == 8){
		printf("GET_FILE\n");

	}

	int err;
	// Serializar la estructura resultado en un JSON string
	char* res_json = serialize_message_to_client(resultado);

	if (res_json == NULL) {
		printf("Error serializando el mensaje\n");
		close(sc);
	}
	else{
		// Enviar el JSON serializado
		err = sendMessage(sc, res_json); // Envía el resultado
		free(res_json); 
		if (err == -1) {
			printf("Error en envío\n");
		}
		close(sc);
	}

	pthread_exit(0);
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
	int sc;
	int val;
	struct peticion pet;
	int err; 
    pthread_attr_t t_attr;	
    pthread_t thid;
	struct args_tratar_msg args_trat_msg;
    char ip_display[INET_ADDRSTRLEN] = "0.0.0.0";

	if (argc != 2){
		printf("Es necesario iniciar el servidor indicando el puerto\n");
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
	server_addr.sin_port        = htons(atoi(argv[1]));

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

    printf("s> init server <%s>: <%d>\n", ip_display, ntohs(server_addr.sin_port));
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

        printf("%s\n", pet_json);


		// Deserializa el mensaje JSON en la estructura 'pet'
		deserialize_message_from_client(pet_json, &pet); // Pasa 'pet_json' directamente
		free(pet_json); 


		memcpy(&args_trat_msg.mess, &pet, sizeof(struct peticion));
		memcpy(&args_trat_msg.client_addr, &client_addr, sizeof(struct sockaddr_in));
		args_trat_msg.sc = sc;

		if (pthread_create(&thid, &t_attr, (void *)tratar_mensaje, (void *)&args_trat_msg) == 0) {
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
    return(0);
}

/* Si se produce Ctrl+C y error: cerramos socket*/
void handle_signal(int sig) {
    if (sig == SIGINT) {
		close(sd);
    } else if (sig == SIGTERM) {
		close(sd);
    }

    // Salir del programa después de recibir la señal
    exit(0);
}