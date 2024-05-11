import socket
import sys
import argparse
import struct
from threading import Thread


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

class P2PClient:
    def __init__(self, server_ip, server_port):
        self.server_ip = server_ip
        self.server_port = server_port
        self.user_name = ""

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
        # Buscar puerto libre, crear socket para recibir peticiones de 
        # otros clientes <<<<<<<<<<<<------------------------------------------------------------------------

        # Crear hilo encargado de escuchar en IP y puerto seleccionado
        # y atender las peticiones


        self.connect_to_server()
        send_message(self.sock, f"CONNECT {username}")
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
        elif result == 1: 
            result = "DISCONNECT FAIL / USER DOES NOT EXIST" 
        elif result == 2: 
            result = "DISCONNECT FAIL / USER NOT CONNECTED"
        elif result == 3: 
            result = "DISCONNECT FAIL"
        self.close_connection()
        return result

    def get_file(self, remote_username, file_name, local_file_name):
        # This method would be responsible for peer-to-peer file transfers
        pass

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
