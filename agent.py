import socket
from getpass import getpass
from sys import argv
import os

HOST = "127.0.0.1"
PORT = 1100

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    s.send("a".encode())

    data = s.recv(2).decode()

    if data[0] == 'y':
        print("Agent accept")
    elif data[0] == 'n':
        print("Agent already work on this computer")
        exit()

    if len(argv) == 2 and 5 <= len(argv[1]) <= 20:
        # print(argv[1])
        s.send(argv[1].encode())
        print("Password accept")
    else:
        while(True):
            password = getpass()
            if 5 <= len(password) <= 20:
                break
            elif len(password) > 20:
                print("Password too long (>20)")
            elif len(password) < 5:
                print("Password too short (< 5)")

        s.send(password.encode())
        print("Password accept")
        
    while (1):
        data = s.recv(2).decode()
        if data[0] == 's':
            os.system("shutdown now")
            exit()

    s.close()
