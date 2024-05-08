import socket
import struct
import sys

arguments = len(sys.argv)
if arguments < 3:
    print('Uso: client_calc  <host> <port>')
    exit()

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

try:
    serverAddress = (sys.argv[1], int(sys.argv[2]))

    a = 10
    b = 500
    op = 0

    data = struct.pack('III', a, b, op)

    sock.sendto(data, serverAddress)
    message, addr = sock.recvfrom(1024)

    res = struct.unpack("I", message)
    print(res)


finally:
    print('closing socket')
    sock.close()
