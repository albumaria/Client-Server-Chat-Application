# Client-Server-Chat-Application
**Chat application that utilizes both TCP and UDP protocols, enabling multiple clients to connect to a server and engage in real-time conversations**

**Made during 1st semester of 2nd Year**

---

The project was created using **Python and C**, with the Server side being implemented on Python and the Client side on C. The steps taken by each side are as follows:

- Server:
  - Establishes TCP socket for all the clients to connect to
  - Maintains a list of all the connected clients' UDP ports on which they will listen on, which is passed to all the connected peers each time one of them disconnects/joins the chat
  - Constantly pings each client, in order to tell which has disconnected
 
- Client:
  - Connects to server using TCP, gaining knowledge of how many clients are currently connected to the chat, and updating the list of the other connected peers
  - Listens for pings, responding to the server, and updates the list when prompted to do so through the messages received
  - Communicates with the other clients using UDP -> Sends messages to all the clients' ports present in the list, receives messages from all other clients
 
For each client to connect to the server, it also needs to specify to the server as an argument the UDP port on which it will listen from. Threads were used for the multi-task aspect of each side.
