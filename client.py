import socket
import sys
import argparse
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
            result_str = "LIST_USERS OK\n"
            for i in (range(num_users)):
                result_str += receive_response(self.sock)
        elif result == 1: 
            result = "LIST_USERS FAIL, USER DOES NOT EXIST" 
        elif result == 2: 
            result = "LIST_USERS FAIL, USER NOT CONNECTED"
        elif result == 3: 
            result = "LIST_USERS FAIL"
        self.close_connection()
        return result

    def disconnect(self, username):
        self.connect_to_server()
        send_message(self.sock, f"DISCONNECT {username}")
        result = receive_response(self.sock)
        self.close_connection()
        return result

    def list_content(self, username, list_user_name):
        self.connect_to_server()
        send_message(self.sock, f"LIST_CONTENT {username} {list_user_name}")
        result = receive_response(self.sock)
        self.close_connection()
        return result

    def get_file(self, remote_username, file_name, local_file_name):
        # This method would be responsible for peer-to-peer file transfers
        pass

    def shell(self):
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
            elif cmd == "CONNECT" and len(command) == 2:
                self.user_name = command[1]
                print(self.connect(command[1]))
            elif cmd == "DISCONNECT" and len(command) == 2:
                print(self.disconnect(command[1]))
            elif cmd == "PUBLISH" and len(command) == 3:
                print(self.publish_content(self.user_name , command[1], command[2]))
            elif cmd == "DELETE" and len(command) == 2:
                print(self.delete_content(self.user_name , command[1]))
            elif cmd == "LIST_USERS" and len(command) == 1:
                print(self.list_users(self.user_name ))
            elif cmd == "LIST_CONTENT" and len(command) == 2:
                print(self.list_content(self.user_name , command[1]))
            elif cmd == "QUIT":
                print("Exiting...")
                break
            else:
                print("Unknown command or incorrect number of arguments")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Client for a P2P File Distribution System')
    parser.add_argument('-s', '--server', required=True, help='Server IP address')
    parser.add_argument('-p', '--port', required=True, type=int, help='Server port')
    args = parser.parse_args()

    client = P2PClient(args.server, args.port)
    client.shell()
