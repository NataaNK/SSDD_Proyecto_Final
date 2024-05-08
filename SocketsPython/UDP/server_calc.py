import socket
import sys
import struct

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

server_address = ('localhost', 10009)
sock.bind(server_address)

while True:
        data, addr = sock.recvfrom(1024)
        print(data, addr)

        a,b,op = struct.unpack("III", data)

        print(a)
        print(b)
        print(op)

        if op==0:
            res = a + b
        else:
            res = a - b

        data = struct.pack("I", res)
        sock.sendto(data, addr)


