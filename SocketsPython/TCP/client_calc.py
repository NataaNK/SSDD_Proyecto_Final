import socket
import sys

def readNumber(sock):
    a = ''
    while True:
        msg = sock.recv(1)
        if (msg == b'\0'):
            break;
        a += msg.decode()

    return(int(a,10))

arguments = len(sys.argv)
if arguments < 3:
    print('Uso: client_calc  <host> <port>')
    exit()

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

server_address = (sys.argv[1], int(sys.argv[2]))
print('connecting to {} port {}'.format(*server_address))
sock.connect(server_address)

try:

    a = 5
    b = 8
    op = 1
    sock.sendall(str(a).encode())
    sock.sendall(b'\0')
    sock.sendall(str(b).encode())
    sock.sendall(b'\0')
    sock.sendall(str(op).encode())
    sock.sendall(b'\0')

    res = readNumber(sock)
    print(res)
finally:
    print('closing socket')
    sock.close()
