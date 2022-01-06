import socket

HOST = "127.0.0.1"
PORT = 1100

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    s.send(bytes("a", 'utf-8'))
    
    data = s.recv(2)
    data = data.decode()

    if data[0] == 'y':
        print("Agent accept")
    elif data[0] == 'n':
        print("Agent already work on this computer")
    
    # while (1):
    #     pass
          
    s.close()
