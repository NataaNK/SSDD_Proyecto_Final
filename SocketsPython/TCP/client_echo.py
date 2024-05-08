import socket
import sys

arguments = len(sys.argv)
if arguments < 3:
    print('Uso: client_echo  <host> <port>')
    exit()

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

server_address = (sys.argv[1], int(sys.argv[2]))
print('connecting to {} port {}'.format(*server_address))
sock.connect(server_address)

try:

    message = b'Esto es una cadena \0'
    sock.sendall(message)

    message = ''
    while True:
        msg = sock.recv(1)
        if (msg == b'\0'):
            break;
        message += msg.decode()

    print('Recibido= ' + message)

finally:
    print('closing socket')
    sock.close()
