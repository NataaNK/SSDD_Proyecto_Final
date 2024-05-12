
## SERVIDOR SOCKETS: Compilación y ejecución
        - make
        - ./server -p < puerto >
        * El servidor se abrirá en ese puerto y localhost del script

## CLIENTE PYTHON SOCKETS: Compilación y ejecución
        - python3 ./client.py -s < ip_server_scokets > -p < puerto_server_sockets >

## SERVIDOR RPC: Compilación y ejecución
        - make -f Makefile.RPC_Print
        - ./RPC_Print_server
        * Se pone en marcha en localhost, por lo tanto, como llamamos a este servidor
          en server.c (socket server), habrá que poner en marcha los dos 
          servidores en la msima IP
        