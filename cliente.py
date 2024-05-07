import socket

def client_program():
    host = '127.0.0.1'  # El servidor está en la misma máquina
    port = 8080  # El puerto que usa el servidor

    client_socket = socket.socket()  # instancia de socket
    client_socket.connect((host, port))  # conectarse al servidor

    message = input(" -> ")  # tomar entrada

    while message.lower().strip() != 'quit':
        client_socket.send(message.encode())  # enviar mensaje
        data = client_socket.recv(1024).decode()  # recibir respuesta

        print('Received from server: ' + data)  # mostrar respuesta

        message = input(" -> ")  # nuevamente tomar entrada

    client_socket.close()  # cerrar la conexión

if __name__ == '__main__':
    client_program()
