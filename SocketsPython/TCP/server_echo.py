import socket
import sys

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

server_address = ('localhost', 10009)
sock.bind(server_address)

sock.listen(5)

while True:
    print('waiting for a connection')
    connection, client_address = sock.accept()
    try:
        print('connection from', client_address)

        message = ''
        while True:
            msg = connection.recv(1)
            if (msg == b'\0'):
                break;
            message += msg.decode()

        print('Recibido= ' + message)
        message = message + "\0"
        connection.sendall(message.encode())

    finally:
        # Clean up the connection
        connection.close()
