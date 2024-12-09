import socket
import struct
import threading
from time import sleep
import select
import sys

def handle_client(client_socket, client_addr):
    # messages that are sent between clients and server:
    # client sends udp port in string format (4 bytes)
    # server sends the number of currently connected clients (integer, 4 bytes)
    # then server send message of format "Join - Client "tcp/ip:port" joined the chat" to every client but the one who joined
    # then the lists are being resent again for each client:
        # first we send the length of the ip address
        # then we send as many bytes as the length of the ip address
        # then we send the port as an integer
    # back in the while, the server repeatedly sends a message 'ping'
    # if the clients responds to it it knows it is alive, so it doesn't do anything, if not, the client has disconnected and we can remove it from the list.
    # if a client disconnects, the lists are being resent to all clients

    # possible messages (as in, strings) the server sends:
    # "Join - Client tcp/ip:port has joined the chat"
    # "ping"
    # "Disconnect - Client tcp/ip:port responded incorrectly. Kill."

    # what the client needs to send to the server:
    # the udp port in string format
    # 1 in response to the ping message
    
    client_addr_str = f"{client_addr[0]}:{client_addr[1]}"
    join_message = f"Join - Client {client_addr_str} joined the chat"
    print(join_message)

    udp_port = client_socket.recv(4).decode()
    print("received udp port: " + udp_port)

    client_data = tuple((client_addr[0], int(udp_port)))
    client_sockets.append(client_socket)
    clients.append(tuple((client_addr[0], int(udp_port))))
    print(clients)
    client_socket.send(struct.pack("!I", len(clients)))
    broadcast(join_message, client_socket)
    send_all_clients()

    while True:
        try:
            if client_socket:
                message_length = len("ping")
            client_socket.send(struct.pack("!I", message_length))
            client_socket.send("ping".encode())

            # Wait for client response
            client_response = client_socket.recv(4)  # getting back an integer to see if it's alive
            if not client_response:
                death_message = f"Disconnect - Client {client_addr_str} died.."
                broadcast(death_message, client_socket)
                print(death_message)
                client_sockets.remove(client_socket)
                clients.remove(client_data)
                print(clients)
                send_all_clients()
                client_socket.close()
                return

            client_response = struct.unpack("!I", client_response)[0]
            if client_response != 1:
                death_message = f"Client {client_addr_str} responded incorrectly. Kill."
                broadcast(death_message, client_socket)
                client_sockets.remove(client_socket)
                clients.remove(client_data)
                print(clients)
                send_all_clients()
                client_socket.close()
                return

            sleep(2)  # wait a bit until the next ping
        except BrokenPipeError:
            # Handle the error gracefully when the client socket is closed
            print(f"Client {client_addr_str} disconnected unexpectedly.")
            death_message = f"Disconnect - Client {client_addr_str} died.."
            broadcast(death_message, client_socket)
            client_sockets.remove(client_socket)
            clients.remove(client_data)
            print(clients)
            send_all_clients()
            client_socket.close()
            return
        except Exception as e:
            print(f"Error: {e}")
            break



def broadcast(message, except_socket):
    message_length = len(message)
    for client in client_sockets:
        if client != except_socket:
            client.send(struct.pack("!I", message_length))
            client.send(message.encode())


def send_all_clients():
    for client in client_sockets:
        for client_addr in clients:
            ip_length = len(client_addr[0])
            client.send(struct.pack("!I", ip_length))
            client.send(client_addr[0].encode())
            client.send(struct.pack("!I", client_addr[1]))
    

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind(('127.0.0.1', 5001))

server_socket.listen(5)
print("Listening for connections")

client_sockets = []
clients = []

while True:
    try:
        client_socket, client_addr = server_socket.accept()
        # print("Received client with address ", client_addr, " creating thread..")
        client_thread = threading.Thread(target=handle_client, args=(client_socket, client_addr))
        client_thread.start()
    except socket.error as e:
        print(f"Error accepting connection: {e}")