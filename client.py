import socket
from getpass import getpass

HOST = "127.0.0.1"
PORT = 1100


def print_commands_info():
    print("")
    print("----------------------- COMANDS -----------------------")
    print("show          - shows list of agents, your permissions and agents status")
    print("add <ip>      - adds permission to given ip")
    print("shutdown <ip> - shut down remotely computer with the given ip")
    print("help          - display commands info")
    print("exit          - turn off this programme")
    print("")

def show():
    s.send("s".encode())

    data = s.recv(4)
    agents_count = int.from_bytes(data, "little", signed="True")

    print(f"Agents list ({agents_count})")
    for i in range(agents_count):
        data = s.recv(32)
        print(data.decode())


def shutdown(msg):
    if len(msg.split(' ')) == 2:

        ipaddr = msg.split(' ')[1]
        # print(ipaddr)

        if len(ipaddr) > 15:
            print("Argument too long (>15)")
            return
        elif len(ipaddr) < 7:
            print("Argument too short (<7)")
            return    

        s.send("d".encode())
        s.send(ipaddr.encode())

        data = s.recv(50)
        print(data.decode())
    else:
        print("Bad argument\nExpected format: shutdown <ip>")


def add(msg):
    if len(msg.split(' ')) == 2:

        ipaddr = msg.split(' ')[1]

        if len(ipaddr) > 15 :
            print("Argument too long (>15)")
            return
        elif len(ipaddr) < 7:
            print("Argument too short (<7)")
            return    

        s.send("a".encode())
        # print(ipaddr.encode())
        s.send(ipaddr.encode())

        data = s.recv(50)
        if data.decode() == 'Password':

            while(True):
                password = getpass()
                if 5 <= len(password) <= 20:
                    break
                elif len(password) > 20:
                    print("Password too long (>20)")
                elif len(password) < 5:
                    print("Password too short (< 5)")

            s.send(password.encode())
            data = s.recv(50)
            print(data.decode())
        else:
            print(data.decode())
    else:
        print("Bad argument\nExpected format: add <ip>")


with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    s.send("c".encode())
    
    print_commands_info()
    while True:
        
        msg = input("rs_client~> ")

        if msg == "exit":
            break
        elif msg == "show":
            show()
        elif msg.split(' ')[0] == 'shutdown':
            shutdown(msg)
        elif msg.split(' ')[0] == 'add':
            add(msg)
        elif msg == "help":
            print_commands_info()
        elif msg == '':
            continue
        else:
            print(f"Command '{msg}' not found")

        print("")   

    s.close()
