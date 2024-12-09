#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <strings.h>

// the messages sent by the client have no special format.

#define MAXCLIENTS 100
int server_socket_fd;
int client_socket_fd;
int length;
pthread_t server_t, write_t, listen_t;
struct sockaddr_in other_clients_receive;
int nr_clients;

void intToStr(int N, char *str) {
    int i = 0;
  
    // Save the copy of the number for sign
    int sign = N;

    // If the number is negative, make it positive
    if (N < 0)
        N = -N;

    // Extract digits from the number and add them to the
    // string
    while (N > 0) {
      
        // Convert integer digit to character and store
      	// it in the str
        str[i++] = N % 10 + '0';
      	N /= 10;
    } 

    // If the number was negative, add a minus sign to the
    // string
    if (sign < 0) {
        str[i++] = '-';
    }

    // Null-terminate the string
    str[i] = '\0';

    // Reverse the string to get the correct order
    for (int j = 0, k = i - 1; j < k; j++, k--) {
        char temp = str[j];
        str[j] = str[k];
        str[k] = temp;
    }
}

typedef struct {
    char ip[16];
    int32_t port;

} ClientAddr;

ClientAddr clients[MAXCLIENTS];

void receive_list(int times) {
    char ip[16];
    int32_t port;
    int nr_bytes;
    int32_t ip_length;
    memset(clients, 0, (sizeof(ClientAddr) * MAXCLIENTS));

    if (times <= 0 || times > MAXCLIENTS) {  // Safety check to avoid out-of-bounds
        fprintf(stderr, "Invalid number of clients received: %d\n", times);
        return;
    }

    for(int i = 0; i < times; i++) {
        memset(ip, 0, 16);
        port = 0;
        ip_length = 0;

        nr_bytes = recv(server_socket_fd, &ip_length, sizeof(int32_t), 0);
        if (nr_bytes <= 0) {
            fprintf(stderr, "Error receiving ip length\n");
            return;
        }
        ip_length = ntohl(ip_length);
        //printf("Received ip length: %d\n", ip_length);

        nr_bytes = recv(server_socket_fd, ip, ip_length, 0);
        if (nr_bytes < 0 || ip_length >= 16) {
            fprintf(stderr, "Error receiving client addr\n");
            return;
        }
        ip[ip_length] = '\0';
        //printf("received ip: %s\n", ip);

        nr_bytes = recv(server_socket_fd, &port, sizeof(int32_t), 0);
        if (nr_bytes < 0) {
            fprintf(stderr, "Error receiving client port\n");
            return;
        }
        port = ntohl(port);
        //printf("Received port: %d\n", port);

        ClientAddr aux;
        strcpy(aux.ip, ip);
        aux.port = port;
        clients[i] = aux;
    }

}

void* server_thread(void* arg) {
    //printf("ENTERED SERVER THREAD\n");
    char message[1024], copy[1024];
    int nr_bytes;
    int32_t message_length;
    
    nr_bytes = recv(server_socket_fd, &nr_clients, sizeof(int32_t), 0);
    if (nr_bytes < 0) {
        fprintf(stderr, "Error receiving nr clients\n");
        return NULL;
    }
    nr_clients = ntohl(nr_clients);
    //printf("NR CLIENTS CONNECTED: %d\n", nr_clients);
    receive_list(nr_clients);

    while(1) {
        memset(message, 0, 1023);
        nr_bytes = recv(server_socket_fd, &message_length, sizeof(int32_t), 0);
        if (nr_bytes < 0) {
            fprintf(stderr, "Error receiving server message length\n");
            break;
        }
        message_length = ntohl(message_length);
        //printf("message length: %d\n", message_length);

        nr_bytes = recv(server_socket_fd, message, message_length, 0);
        if (nr_bytes < 0) {
            fprintf(stderr, "Error receiving server message\n");
            break;
        }
        strcpy(copy, message);

        if (strcmp(copy, "ping") == 0) {  //case when the message is 'ping'
            int32_t yes = 1;
            yes = htonl(yes);
            nr_bytes = send(server_socket_fd, &yes, sizeof(int32_t), 0);
        }
        else if (strcmp(strtok(copy, " "), "Join") == 0){  // otherwise, its a connection/log out message (so we need to receive the updated client list)
            printf("%s\n", message);
            nr_clients++;
            //printf("nr clients: %d\n", nr_clients);
            receive_list(nr_clients);
            
        }
        else if (strcmp(strtok(copy, " "), "Disconnect") == 0) {
            printf("%s\n", message);
            nr_clients--;
            //printf("nr clients: %d\n", nr_clients);
            receive_list(nr_clients);

        }
         
    }

    return NULL;
}

void* listen_thread(void* arg) {
    //printf("ENTERED LISTEN THREAD\n");
    char message[1024];
    int nr_bytes;

    while(1) {
        memset(message, 0, 1023);

        nr_bytes = recvfrom(client_socket_fd, message, 1023, 0, (struct sockaddr *)&other_clients_receive, &length);

        if (nr_bytes < 0) {
            fprintf(stderr, "Blank\n");
        }
        char* client_ip = inet_ntoa(other_clients_receive.sin_addr);
        unsigned short client_port = ntohs(other_clients_receive.sin_port);

        printf("<%s:%u> -> %s\n", client_ip, client_port, message);
    }


    return NULL;
}

void* write_thread(void* arg) {
    //printf("ENTERED WRITE THREAD\n");

    while(1) {
        char message[1024];
        fgets(message, sizeof(message), stdin);
        message[strlen(message) - 1] = '\0';
        //printf("Message: %s\n", message);
        int send_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (send_socket_fd < 0) {
            perror("Error creating send socket");                
            continue;
        }

        for (int i = 0; i < nr_clients; i++) {
            //printf("%s %d\n", clients[i].ip, clients[i].port);
            struct sockaddr_in client_addr;
            memset(&client_addr, 0, length);
            client_addr.sin_family = AF_INET;
            client_addr.sin_port = htons(clients[i].port);
            client_addr.sin_addr.s_addr = inet_addr(clients[i].ip);

            int nr_bytes = sendto(send_socket_fd, message, strlen(message), 0, (struct sockaddr *)&client_addr, length);
            //printf("nr bytes sent %d\n", nr_bytes);
            if (nr_bytes < 0) {
                fprintf(stderr, "Error sending message to client %s:%d\n", clients[i].ip, clients[i].port);
            }
        }
    }

    return NULL;
}

void tratare() {

    int result = pthread_create(&server_t, NULL, server_thread, NULL);
    if (result != 0) {
       fprintf(stderr, "Error creating thread %d\n", result);
    }

    result = pthread_create(&listen_t, NULL, listen_thread, NULL);
    if (result != 0) {
       fprintf(stderr, "Error creating listen thread %d\n", result);
    }

    result = pthread_create(&write_t, NULL, write_thread, NULL);
    if (result != 0) {
       fprintf(stderr, "Error creating write thread %d\n", result);
    }
}


int main(int argc, char* argv[]) {
    // now argv[1] is gonna be where the udp port is bound and it needs to send that to the server
    memset(clients, 0, (sizeof(ClientAddr) * MAXCLIENTS));
    struct sockaddr_in server;
    int code;

    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket_fd < 0) {
        fprintf(stderr, "Error creating client socket\n");
        return 1;
    }

    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(5001);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    code = connect(server_socket_fd, (struct sockaddr*) &server, sizeof(struct sockaddr_in));
    if (code < 0) {
        fprintf(stderr, "Error connecting to server socket\n");
        return 1;
    }

    printf("Connected to server\n");

    
    client_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(client_socket_fd < 0) {
        fprintf(stderr, "Error creating the UDP socket\n");
        return 1;
    }

    length = sizeof(other_clients_receive);
    memset(&other_clients_receive, 0, length);
    other_clients_receive.sin_family = AF_INET;
    other_clients_receive.sin_addr.s_addr = INADDR_ANY;
    other_clients_receive.sin_port = htons(atoi(argv[1]));

    bind(client_socket_fd, (struct sockaddr *)&other_clients_receive, length);

    int32_t udp_port_nr = atoi(argv[1]);
    printf("port: %d\n", udp_port_nr);
    char port_in_char[4];
    intToStr(udp_port_nr, port_in_char);
    port_in_char[4] = '\0';

    //printf("port in char : %s\n", port_in_char);

    // sending udp port to the server first thing
    int nr_bytes = send(server_socket_fd, port_in_char, 4, 0);
    if (nr_bytes < 0) {
        fprintf(stderr, "Error sending udp port\n");
        return 1;
    }

    tratare();

    pthread_join(server_t, NULL);
    pthread_join(write_t, NULL);
    pthread_join(listen_t, NULL);

    //close(socket_fd);
}