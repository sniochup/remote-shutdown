#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 1100

struct clag //client or agent
{
    int socket;
    char * address;
    int connect;
};

struct clag *clients;
struct clag *agents;
struct clag new_clag(int socket, char *address);

void *client_thread(void *arg);

int agents_count;

int main(void)
{
    int server_socket, client_socket;
    pthread_t thread_id;
    
    struct sockaddr_in addr_storage;
    socklen_t addr_size = sizeof addr_storage;
    
    char * client_address;
    char who[2];

    clients = (struct clag*)malloc(10 * sizeof(struct clag));
    int clients_count = 0;
    int c_size = 10;

    agents = (struct clag*)malloc(10 * sizeof(struct clag));
    agents_count = 0;
    int a_size = 10;
    
    char a_msg[2];

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof server_addr);


    if ((server_socket = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Can't create socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof server_addr) == -1) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 50) == -1) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        if ((client_socket = accept(server_socket, (struct sockaddr *)&addr_storage, &addr_size)) == -1) {
            perror("Can't accept socket");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        client_address = inet_ntoa((struct in_addr)addr_storage.sin_addr);

        if (recv(client_socket, who, 2, 0) == -1) {
            perror("Recv problem");
            close(client_socket);
            close(server_socket);
            exit(EXIT_FAILURE);
        }
        
        if (*who == 'c'){
            // zarządzanie pamięcią clientów
            if (clients_count == c_size) {
                int temp = (clients_count/10+1)*10;
                clients = (struct clag*)realloc(clients, temp * sizeof(struct clag));
                c_size += 10;

                for (int i = 0; i < clients_count; i++) {
                    printf("Client %d. %s : %d\n", i, clients[i].address, clients[i].socket);
                }
            }

            // Szukanie clienta 
            int c_exist = 1;

            // for (int i = 0; i < clients_count; i++) {
            //     c_exist = strcmp(clients[i].address, client_address);
            //     if (c_exist == 0) {
            //         printf("Client connection - %s : %d\n", clients[i].address, clients[i].socket);

            //         pthread_create(&thread_id, NULL, client_thread, &clients[i]);
            //         pthread_detach(thread_id);

            //         break;
            //     }
            // }

            // Tworzenie nowych clientów
            if (c_exist == 1) {
                clients[clients_count] = new_clag(client_socket, client_address);
                printf("Client connection - %s : %d\n", clients[clients_count].address, clients[clients_count].socket);
                pthread_create(&thread_id, NULL, client_thread, &clients[clients_count]);
                pthread_detach(thread_id);

                clients_count++;
            }
        }
        else if (*who == 'a') {
            if (agents_count == a_size){
                int temp = (agents_count/10+1)*10;
                agents = (struct clag*)realloc(agents, temp * sizeof(struct clag));
                a_size += 10;

                for (int i = 0; i<agents_count; i++) {
                    printf("AGENT - %d: %s %d\n", i, agents[i].address, agents[i].socket);
                }
            }
            int a_exist = 1;
            for (int i = 0; i < agents_count; i++) {
                a_exist = strcmp(agents[i].address, client_address);
                if (a_exist == 0) {
                    printf("Agent from %s already connected\n", agents[i].address);
                    printf("1");
                    strcpy(a_msg, "n");
                    printf("1");
                    send(client_socket, a_msg, 2, 0);
                    printf("1");
                    break;
                }
            }

            if (a_exist == 1) {
                agents[agents_count] = new_clag(client_socket, client_address);
                printf("Agents connection - %s : %d\n", agents[agents_count].address, agents[agents_count].socket);
                strcpy(a_msg, "y");
                send(client_socket, a_msg, 2, 0);

                pthread_create(&thread_id, NULL, client_thread, &agents[agents_count]);
                pthread_detach(thread_id);

                agents_count++;
            }
        }

        // check((pthread_create(&thread_id, NULL, client_thread, (void *)sthread) != 0), "Failed to create thread\n", 0);
        // pthread_detach(thread_id);
    }

    close(server_socket);
    return EXIT_SUCCESS;
}

struct clag new_clag(int cs, char *ca) {
    struct clag *clag_temp = (struct clag *)malloc(sizeof(struct clag));
    clag_temp->address = ca;
    clag_temp->socket = cs;
    clag_temp->connect = 1;
    return *clag_temp;
}

void *client_thread(void *arg)
{
    int newSocket = ((struct clag*)arg)->socket;
    int n;
    char msg[2];
    char message[32];

    while (1) {
        n = recv(newSocket, msg, 2, 0);

        if (n == 0) {
            printf("Client disconect - %s : %d\n", ((struct clag*)arg)->address, ((struct clag*)arg)->socket);
            break;
        }
        else if (*msg == 's') {
            send(newSocket, &agents_count, sizeof(agents_count), 0);
            for (int i = 0; i < agents_count; i++) {
                n = sprintf(message, "%d. %s : p\n", i+1, agents[i].address);
                send(newSocket, message, n, 0);
            }
        }
        else if (*msg == 'd') {
            send(newSocket, "ja cie zara wylacze", 256, 0);
        }

        printf("%s\n", msg);

        // char *message = (char *)malloc(sizeof(client_message));
        // strcpy(message, client_message);
        // send(newSocket, message, 256, 0);

        memset(&msg, 0, sizeof(msg));
        memset(&message, 0, sizeof(message));
    }

    pthread_exit(NULL);
}