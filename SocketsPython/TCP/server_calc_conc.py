import threading
import socket

def readNumber(sock):
    a = ''
    while True:
        msg = sock.recv(1)
        if (msg == b'\0'):
            break;
        a += msg.decode()

    return(int(a,10))


def worker(sock):
    try:
        a = readNumber(sock)
        b = readNumber(sock)
        op = readNumber(sock)


        if (op == 0):
            res = a + b
        else:
            res = a - b

        message = str(res) + "\0"
        message = message + "\0"
        sock.sendall(message.encode())
    finally:
        # Clean up the connection
        sock.close()


sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

server_address = ('localhost', 10009)
sock.bind(server_address)

sock.listen(5)

while True:
    print('waiting for a connection')
    connection, client_address = sock.accept()
    print('connection from', client_address)

    t = threading.Thread(target=worker, name='Daemon', args=(connection,))
    t.start()



