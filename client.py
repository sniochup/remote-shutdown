import socket

HOST = "127.0.0.1"
PORT = 1100

def print_comands_info():
    print("")
    print("------------------- COMANDS -----------------------------------------")
    print("show                   - show list of agents and your permissions")
    print("shutdown <computer ip> - shut down remotely computer with the given ip")
    print("")


with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    s.send(bytes("c", 'utf-8'))
    
    print_comands_info()
    while True:
        
        msg = input("rs_client~> ")

        if msg == "exit":
            break

        elif msg == "show":
            s.send(bytes("s", 'utf-8'))
            data = s.recv(1)
            agents_count = int.from_bytes(data, "little", signed="True")

            print("Agents list:")
            for i in range(agents_count):
                data = s.recv(32)
                print(data.decode())
                

        elif msg.split(' ')[0] == 'shutdown':
            s.send(bytes("d", 'utf-8'))

            data = s.recv(2048)
            print(data.decode())
            pass

        else:
            print(f"Command '{msg}' not found")

    s.close()
