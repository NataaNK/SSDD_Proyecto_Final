import socket
import sys
import argparse
from threading import Thread

def send_message(sock, message):
    sock.sendall(f"{message}\0".encode())

def receive_response(sock):
    response = ""
    while True:
        chunk = sock.recv(1)
        if chunk == b'\0':
            break
        response += chunk.decode()
    return response

class P2PClient:
    def __init__(self, server_ip, server_port):
        self.server_ip = server_ip
        self.server_port = server_port

    def connect_to_server(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.server_ip, int(self.server_port)))

    def close_connection(self):
        self.sock.close()

    def register(self, username):
        self.connect_to_server()
        send_message(self.sock, f"REGISTER {username}")
        result = receive_response(self.sock)
        self.close_connection()
        return result

    def unregister(self, username):
        self.connect_to_server()
        send_message(self.sock, f"UNREGISTER {username}")
        result = receive_response(self.sock)
        self.close_connection()
        return result

    def connect(self, username):
        self.connect_to_server()
        send_message(self.sock, f"CONNECT {username}")
        result = receive_response(self.sock)
        self.close_connection()
        return result

    def disconnect(self, username):
        self.connect_to_server()
        send_message(self.sock, f"DISCONNECT {username}")
        result = receive_response(self.sock)
        self.close_connection()
        return result

    def publish_content(self, username, file_name, description):
        self.connect_to_server()
        send_message(self.sock, f"PUBLISH {username} {file_name} {description}")
        result = receive_response(self.sock)
        self.close_connection()
        return result

    def delete_content(self, username, file_name):
        self.connect_to_server()
        send_message(self.sock, f"DELETE {username} {file_name}")
        result = receive_response(self.sock)
        self.close_connection()
        return result

    def list_users(self, username):
        self.connect_to_server()
        send_message(self.sock, f"LIST_USERS {username}")
        result = receive_response(self.sock)
        self.close_connection()
        return result

    def list_content(self, username):
        self.connect_to_server()
        send_message(self.sock, f"LIST_CONTENT {username}")
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
                print(self.register(command[1]))
            elif cmd == "UNREGISTER" and len(command) == 2:
                print(self.unregister(command[1]))
            elif cmd == "CONNECT" and len(command) == 2:
                print(self.connect(command[1]))
            elif cmd == "DISCONNECT" and len(command) == 2:
                print(self.disconnect(command[1]))
            elif cmd == "PUBLISH" and len(command) == 4:
                print(self.publish_content(command[1], command[2], command[3]))
            elif cmd == "DELETE" and len(command) == 3:
                print(self.delete_content(command[1], command[2]))
            elif cmd == "LIST_USERS" and len(command) == 2:
                print(self.list_users(command[1]))
            elif cmd == "LIST_CONTENT" and len(command) == 2:
                print(self.list_content(command[1]))
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
