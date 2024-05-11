import errno
import socket
import sys
import argparse
import struct
from threading import Thread
import socket
import threading
from random import randint
import os



def send_message(sock, message):
    message_encoded = message.encode()  # Codificar el mensaje en bytes
    length = len(message_encoded)
    sock.sendall(length.to_bytes(4, byteorder='big'))  # Enviar la longitud como 4 bytes
    sock.sendall(message_encoded)  # Enviar el mensaje codificado


def receive_response(sock):
    length_bytes = sock.recv(4)
    if not length_bytes:
        return "CONNECTION ERROR"  # Manejar caso de desconexión
    # Convertir los bytes a entero
    response = int.from_bytes(length_bytes, byteorder='big')
    return response

def receive_user_list(sock):
    """
    Receive a list of user details from the server. Assuming that the server could send multiple user details at once.
    
    Args:
    sock (socket.socket): The socket object used for the communication.
    
    Returns:
    list of str: A list of formatted strings, each containing one user's key and values.
    """
    try:
        data = sock.recv(4096).decode('utf-8')  # Receives data from the server
        if data:
            users = []
            # Split data into separate lines
            lines = data.strip().split('\n')
            for line in lines:
                items = line.split()
                if items:
                    key = items[0]
                    values = ' '.join(items[1:])
                    users.append(f"{key} {values}")
            return users
        else:
            return []
    except socket.error as e:
        print(f"Socket error: {e}")
        return []
    
def receive_content_list(sock, num_contents):
    """
    Receive a specified number of content items from a socket, and process them to format quotes.

    Args:
        sock (socket.socket): The socket object used for communication.
        num_contents (int): The number of content items to receive.

    Returns:
        list of str: A list of content items received from the server, with quotes formatted.
    """
    contents = []
    for _ in range(num_contents):
        content = receive_line(sock).strip()  # Remove any trailing newline or extra spaces
        # Remove double quotes and replace single quotes with double quotes
        formatted_content = content.replace('"', '').replace("'", '"')
        contents.append(formatted_content)
    return contents


def receive_line(sock):
    """
    Helper function to receive one line of text from a socket.

    Args:
        sock (socket.socket): The socket object used for communication.

    Returns:
        str: A single line of text received from the socket.
    """
    line = b""
    while True:
        part = sock.recv(1)
        if part == b'\n':
            break
        line += part
    return line.decode('utf-8')

def find_free_port():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(("", 0))  # Bind to an available port provided by the host.
    port = s.getsockname()[1]  # Retrieve the port number
    s.close()
    return port

def receive_file_from_host(ip, port, remote_file_name, local_file_name):
    try:
        # Conectar al host remoto
        remote_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        remote_sock.connect((ip, int(port)))
        
        # Enviar solicitud de archivo
        request_message = f"GET_FILE {remote_file_name}"
        remote_sock.sendall(request_message.encode())

        # Recibir la respuesta del servidor remoto
        server_response = remote_sock.recv(1)
        response_code = int.from_bytes(server_response, byteorder='big')

        if response_code == 0:
            # Recibir y escribir el archivo
            with open(local_file_name, 'wb') as file:
                while True:
                    data = remote_sock.recv(4096)
                    if not data:
                        break
                    file.write(data)
            return "GET_FILE OK"
        elif response_code == 1:
            if os.path.exists(local_file_name):
                os.remove(local_file_name)  # Elimina el archivo local si la descarga no fue exitosa
            return "GET_FILE FAIL / FILE NOT EXIST"
        else:
            if os.path.exists(local_file_name):
                os.remove(local_file_name)  # Elimina el archivo local si la descarga no fue exitosa
            return "GET_FILE FAIL"

    except Exception as e:
        print(e)
        sys.stdout.flush()
        if os.path.exists(local_file_name):
            os.remove(local_file_name)  # Asegúrate de no dejar un archivo parcialmente descargado
        return "GET_FILE FAIL"
    finally:
        remote_sock.close()  # Asegúrate de cerrar la conexión independientemente del resultado  

def process_request(client_sock):
    # Recibir una solicitud de fichero y enviarlo.
    try:
        # Recibir la solicitud
        request = client_sock.recv(1024).decode('utf-8')
        print(f"Received request: {request}")
        parts = request.split()
        command, file_name = parts[0], parts[1]

        # Verificar si el archivo existe
        if os.path.exists(file_name):
            # Preparar el archivo para enviar
            with open(file_name, 'rb') as file:
                # Notificar al cliente que el archivo será enviado
                client_sock.sendall(b'\x00')  # Código de operación: 0 -> archivo se transferirá
                # Enviar el contenido del archivo
                data = file.read(4096)
                while data:
                    client_sock.sendall(data)
                    data = file.read(4096)
            sys.stdout.write("File sent successfully.\nc> ")
        else:
            # Notificar al cliente que el archivo no existe
            client_sock.sendall(b'\x01')  # Código de operación: 1 -> archivo no existe
            sys.stdout.write("File does not exist.\nc> ")

    except Exception as e:
        print(f"Error handling request: {e}")
        client_sock.sendall(b'\x02')  # Código de operación: 2 -> error general

    finally:
        client_sock.close()


class P2PClient:
    def __init__(self, server_ip, server_port):
        self.server_ip = server_ip
        self.server_port = server_port
        self.user_name = ""
        self.listen_socket = None
        self.listen_thread = None
        self.stop_event = threading.Event()

    def connect_to_server(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.server_ip, self.server_port))

    def close_connection(self):
        self.sock.close()

    def register(self, username):
        self.connect_to_server()
        send_message(self.sock, f"REGISTER {username}")
        result = receive_response(self.sock)
        if result == 0:
            result = "REGISTER OK"
        elif result == 1: 
            result = "USERNAME IN USE" 
        elif result == 2: 
            result = "REGISTER FAIL"
        self.close_connection()
        return result

    def unregister(self, username):
        self.connect_to_server()
        send_message(self.sock, f"UNREGISTER {username}")
        result = receive_response(self.sock)
        if result == 0:
            result = "UNREGISTER OK"
        elif result == 1: 
            result = "USER DOES NOT EXIST" 
        elif result == 2: 
            result = "UNREGISTER FAIL"
        self.close_connection()
        return result

    def connect(self, username):
        # Paso 1: Encontrar un puerto libre y crear un socket para escuchar peticiones
        self.listen_port = find_free_port()
        self.listen_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.listen_socket.bind(("", self.listen_port))
        self.listen_socket.listen()

        # Paso 2: Crear un hilo para escuchar y atender peticiones
        self.stop_event.clear()
        self.listen_thread = threading.Thread(target=self.handle_client_requests)
        self.listen_thread.start()

        # Paso 3: Conectarse al servidor central
        self.connect_to_server()
        send_message(self.sock, f"CONNECT {username} {self.listen_port}")
        result = receive_response(self.sock)
        if result == 0:
            result = "CONNECT OK"
        elif result == 1: 
            result = "CONNECT FAIL, USER DOES NOT EXIST" 
        elif result == 2: 
            result = "USER ALREADY CONNECTED"
        elif result == 3: 
            result = "CONNECT FAIL"

        self.close_connection()
        return result
    
    def handle_client_requests(self):
        self.listen_socket.settimeout(1)  # Establece un timeout de 1 segundo
        while not self.stop_event.is_set():
            try:
                client_sock, addr = self.listen_socket.accept()
                print(f"Accepted connection from {addr}")
                threading.Thread(target=process_request, args=(client_sock,)).start()
            except socket.timeout:
                continue  # Continúa revisando si el evento de parada está señalizado
            except socket.error as e:
                if e.errno == errno.EBADF:
                    break  # Sale del bucle si el socket está cerrado
                else:
                    print(f"Error during accept or handling: {e}")


    def publish_content(self, username, file_name, description):
        self.connect_to_server()
        send_message(self.sock, f"PUBLISH {username} {file_name} {description}")
        result = receive_response(self.sock)
        if result == 0:
            result = "PUBLISH OK"
        elif result == 1: 
            result = "PUBLISH FAIL, USER DOES NOT EXIST" 
        elif result == 2: 
            result = "PUBLISH FAIL, USER NOT CONNECTED"
        elif result == 3: 
            result = "PUBLISH FAIL, CONTENT ALREADY PUBLISHED"
        elif result == 4: 
            result = "PUBLISH FAIL"
        self.close_connection()
        return result

    def delete_content(self, username, file_name):
        self.connect_to_server()
        send_message(self.sock, f"DELETE {username} {file_name}")
        result = receive_response(self.sock)
        if result == 0:
            result = "DELETE OK"
        elif result == 1: 
            result = "DELETE FAIL, USER DOES NOT EXIST" 
        elif result == 2: 
            result = "DELETE FAIL, USER NOT CONNECTED"
        elif result == 3: 
            result = "DELETE FAIL, CONTENT NOT PUBLISHED"
        elif result == 4: 
            result = "DELETE FAIL"
        self.close_connection()
        return result

    def list_users(self, username):
        self.connect_to_server()
        send_message(self.sock, f"LIST_USERS {username}")
        result = receive_response(self.sock)
        if result == 0:
            num_users = receive_response(self.sock)
            result_str = "c> LIST_USERS OK\n"
            all_users = receive_user_list(self.sock)
            for user_info in all_users[:num_users]:  # Limit to expected number of users
                result_str += '\t' + user_info + '\n'
            result = result_str.rstrip('\n') # Eliminar último /n
        elif result == 1: 
            result = "LIST_USERS FAIL, USER DOES NOT EXIST" 
        elif result == 2: 
            result = "LIST_USERS FAIL, USER NOT CONNECTED"
        elif result == 3: 
            result = "LIST_USERS FAIL"
        self.close_connection()
        return result
    
    def list_content(self, username, list_user_name):
        self.connect_to_server()
        send_message(self.sock, f"LIST_CONTENT {username} {list_user_name}")
        result = receive_response(self.sock)
        if result == 0:
            num_contents = receive_response(self.sock)
            result_str = "c> LIST_CONTENT OK\n"
            all_content = receive_content_list(self.sock, num_contents)
            for content_info in all_content:
                result_str += '\t' + content_info + '\n'
            result = result_str.rstrip('\n')  # Eliminar último '\n'
        elif result == 1: 
            result = "LIST_CONTENT FAIL, USER DOES NOT EXIST" 
        elif result == 2: 
            result = " LIST_CONTENT FAIL, USER NOT CONNECTED"
        elif result == 3: 
            result = "LIST_CONTENT FAIL, REMOTE USER DOES NOT EXIST"
        elif result == 4: 
            result = "LIST_CONTENT FAIL"
        self.close_connection()
        return result

    def disconnect(self, username):
        self.connect_to_server()
        send_message(self.sock, f"DISCONNECT {username}")
        result = receive_response(self.sock)

        if result == 0:
            result = "DISCONNECT OK"
            # Cerrar primero el socket de escucha
            self.listen_socket.close()
            self.stop_event.set()
            self.listen_thread.join()  # Espera a que el hilo de escucha termine
        elif result == 1: 
            result = "DISCONNECT FAIL / USER DOES NOT EXIST" 
        elif result == 2: 
            result = "DISCONNECT FAIL / USER NOT CONNECTED"
        elif result == 3: 
            result = "DISCONNECT FAIL"                
            self.listen_socket.close()
            self.stop_event.set()
            self.listen_thread.join()
        self.close_connection()
        return result

    def get_file(self, username, remote_username, remote_file_name, local_file_name):
        self.connect_to_server()
        send_message(self.sock, f"GET_FILE {username} {remote_username} {remote_file_name} {local_file_name}")
        result = receive_response(self.sock)
        if result == 1:
            result = "GET_FILE FAIL, USER DOES NOT EXIST"
        elif result == 2: 
            result = "GET_FILE FAIL, USER NOT CONNECTED" 
        elif result == 3: 
            result = "GET_FILE FAIL"
        elif result == 4: 
            result = "GET_FILE FAIL, REMOTE USER DOES NOT EXIST"
        elif result == 5: 
            result = "GET_FILE FAIL, REMOTE USER NOT CONNECTED"
        elif result == 6:
            result = "GET_FILE FAIL / FILE NOT EXIST"
        else:
            ip_port_data = self.sock.recv(1024).decode().rstrip("\x00")
            ip, port = ip_port_data.split()
            # CONECTARSE A CLIENTE REMOTO Y DESCARGAR ARCHIVO
            remote_file_name = os.path.abspath(remote_file_name)
            local_file_name = os.path.abspath(local_file_name)
            result = receive_file_from_host(ip, port, remote_file_name, local_file_name)
        
        self.close_connection()
        return result

    def shell(self):
        try:
            while True:
                    command = input("c> ").strip().split()
                    if not command:
                        continue
                    cmd = command[0].upper()
                    if cmd == "REGISTER" and len(command) == 2:
                        self.user_name = command[1]
                        print(self.register(command[1]))
                    elif cmd == "UNREGISTER" and len(command) == 2:
                        print(self.unregister(command[1]))
                        self.disconnect(command[1])
                    elif cmd == "CONNECT" and len(command) == 2:
                        self.user_name = command[1]
                        print(self.connect(command[1]))
                    elif cmd == "DISCONNECT" and len(command) == 2:
                        print(self.disconnect(command[1]))
                    elif cmd == "PUBLISH" and len(command) >= 3:
                        descripcion = "'" + command[2]
                        for palabra in range(len(command)-3):
                            descripcion += " " + command[palabra+3]
                        descripcion += "'"
                        print(self.publish_content(self.user_name , command[1], descripcion))
                    elif cmd == "DELETE" and len(command) == 2:
                        print(self.delete_content(self.user_name , command[1]))
                    elif cmd == "LIST_USERS" and len(command) == 1:
                        print(self.list_users(self.user_name ))
                    elif cmd == "LIST_CONTENT" and len(command) == 2:
                        print(self.list_content(self.user_name , command[1]))
                    elif cmd == "GET_FILE" and len(command) == 4:
                        print(self.get_file(self.user_name, command[1], command[2], command[3]))
                    elif cmd == "QUIT":
                        self.disconnect(self.user_name)
                        print("Exiting...")
                        break
                    else:
                        print("Unknown command or incorrect number of arguments")

        except KeyboardInterrupt:
            self.disconnect(self.user_name)
      
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Client for a P2P File Distribution System')
    parser.add_argument('-s', '--server', required=True, help='Server IP address')
    parser.add_argument('-p', '--port', required=True, type=int, help='Server port')
    args = parser.parse_args()

    client = P2PClient(args.server, args.port)
    client.shell()
