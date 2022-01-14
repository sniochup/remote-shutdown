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

struct clst // client struct
{
    int socket;
    char *address;
    char **permissions;
    int permissions_counter;
};

struct agst // agent struct
{
    int socket;
    char *address;
    int connect;
    char *pass;
};

struct clst *clients;
struct agst *agents;

int agents_count;

struct clst new_clst(int socket, char *address);
struct agst new_agst(int socket, char *address);

void *client_thread(void *arg);
void *agents_thread(void *arg);

// command service
void *show(int cnum);
void *sdown(int cnum);
void *add(int cnum);

int main(void)
{
    int server_socket, client_socket;
    pthread_t thread_id;

    struct sockaddr_in addr_storage;
    socklen_t addr_size = sizeof addr_storage;

    char *client_address;
    char who[2];

    clients = (struct clst *)malloc(10 * sizeof(struct clst));
    int clients_count = 0;
    int c_size = 10;

    agents = (struct agst *)malloc(10 * sizeof(struct agst));
    agents_count = 0;
    int a_size = 10;

    char a_msg[2];

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof server_addr);

    if ((server_socket = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Can't create socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof server_addr) == -1)
    {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 50) == -1)
    {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Listening...\n\n");
    while (1)
    {
        if ((client_socket = accept(server_socket, (struct sockaddr *)&addr_storage, &addr_size)) == -1)
        {
            perror("Can't accept socket");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        client_address = inet_ntoa((struct in_addr)addr_storage.sin_addr);

        if (recv(client_socket, who, 2, 0) == -1)
        {
            perror("Recv problem");
            close(client_socket);
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        // Obsługa clienta
        if (*who == 'c')
        {
            // zarządzanie pamięcią klientów
            if (clients_count == c_size)
            {
                int temp = (clients_count / 10 + 1) * 10;
                clients = (struct clst *)realloc(clients, temp * sizeof(struct clst));
                c_size += 10;

                for (int i = 0; i < clients_count; i++)
                {
                    printf("Client %d. %s : %d : %d\n", i, clients[i].address, clients[i].socket, clients[i].permissions_counter);
                }
            }

            clients[clients_count] = new_clst(client_socket, client_address);
            printf("Client connection - %s : %d\n", clients[clients_count].address, clients[clients_count].socket);

            int c = clients_count;
            if (pthread_create(&thread_id, NULL, client_thread, &c) != 0)
            {
                perror("Failed to create thread");
            }
            pthread_detach(thread_id);

            clients_count++;
        }
        // Obsługa agenta
        else if (*who == 'a')
        {
            // zarządzanie pamięcią agentów
            if (agents_count == a_size)
            {
                int temp = (agents_count / 10 + 1) * 10;
                agents = (struct agst *)realloc(agents, temp * sizeof(struct agst));
                a_size += 10;

                for (int i = 0; i < agents_count; i++)
                {
                    printf("Agent %d. %s : %d\n", i, agents[i].address, agents[i].socket);
                }
            }

            int a_exist = 1;
            for (int i = 0; i < agents_count; i++)
            {
                a_exist = strcmp(agents[i].address, client_address);
                if (a_exist == 0 && agents[i].connect == 1)
                {
                    printf("Agent from %s already connected\n", agents[i].address);
                    strcpy(a_msg, "n");
                    send(client_socket, a_msg, 2, 0);
                    break;
                }
                else if (a_exist == 0)
                {
                    agents[i] = new_agst(client_socket, client_address);
                    printf("Agents connection - %s : %d\n", agents[i].address, agents[i].socket);
                    strcpy(a_msg, "y");
                    send(client_socket, a_msg, 2, 0);
                    if (pthread_create(&thread_id, NULL, agents_thread, &i) != 0)
                    {
                        perror("Failed to create thread");
                    }
                    pthread_detach(thread_id);
                    break;
                }
            }

            if (a_exist == 1)
            {
                agents[agents_count] = new_agst(client_socket, client_address);
                printf("Agents connection - %s : %d\n", agents[agents_count].address, agents[agents_count].socket);
                strcpy(a_msg, "y");
                send(client_socket, a_msg, 2, 0);
                int a = agents_count;
                if (pthread_create(&thread_id, NULL, agents_thread, &a) != 0)
                {
                    perror("Failed to create thread");
                }
                pthread_detach(thread_id);

                agents_count++;
            }
        }
    }

    close(server_socket);
    return EXIT_SUCCESS;
}

struct clst new_clst(int cs, char *ca)
{
    struct clst *clst_temp = (struct clst *)malloc(sizeof(struct clst));
    clst_temp->address = ca;
    clst_temp->socket = cs;
    clst_temp->permissions = NULL;
    clst_temp->permissions_counter = 0;

    return *clst_temp;
}

struct agst new_agst(int cs, char *ca)
{
    struct agst *agst_temp = (struct agst *)malloc(sizeof(struct agst));
    agst_temp->address = ca;
    agst_temp->socket = cs;
    agst_temp->connect = 1;
    agst_temp->pass = NULL;

    return *agst_temp;
}

void *client_thread(void *arg)
{
    int cnum = *((int *)arg);
    int n;
    char msg[1];

    while (1)
    {
        n = recv(clients[cnum].socket, msg, sizeof(msg), 0);

        if (n == 0)
        {
            printf("Client disconect - %s : %d\n", clients[cnum].address, clients[cnum].socket);
            break;
        }
        else if (*msg == 's')
            show(cnum);
        else if (*msg == 'd')
            sdown(cnum);
        else if (*msg == 'a')
            add(cnum);

        memset(&msg, 0, sizeof(msg));
    }

    pthread_exit(NULL);
}

void *show(int cnum)
{
    printf("SHOW [%d] agents for %s : %d\n", agents_count, clients[cnum].address, clients[cnum].socket);

    int n;
    char permit;
    char message[32];

    send(clients[cnum].socket, &agents_count, sizeof(agents_count), 0);
    for (int i = 0; i < agents_count; i++)
    {
        permit = 'N';
        for (int j = 0; j < clients[cnum].permissions_counter; j++)
        {
            if (strcmp(agents[i].address, clients[cnum].permissions[j]) == 0)
            {
                permit = 'P';
                break;
            }
        }
        n = sprintf(message, "%d. %s : %c : %d", i + 1, agents[i].address, permit, agents[i].connect);
        send(clients[cnum].socket, message, n, 0);
        sleep(0.01);
        memset(&message, 0, sizeof(message));
    }

    return 0;
}

void *sdown(int cnum)
{
    int n;
    int a_exist = 0;
    int p_exist = 0;
    char agent_ip[16];
    char message[50];
    char msg[2];

    memset(&agent_ip, 0, sizeof(agent_ip));
    memset(&message, 0, sizeof(message));
    memset(&msg, 0, sizeof(msg));

    n = recv(clients[cnum].socket, agent_ip, sizeof(agent_ip), 0);
    printf("SHUTDOWN agent ip %s - for %s : %d\n", agent_ip, clients[cnum].address, clients[cnum].socket);

    for (int i = 0; i < agents_count; i++)
    {
        if (strcmp(agent_ip, agents[i].address) == 0 && agents[i].connect == 1)
        {
            a_exist = 1;
            p_exist = 0;
            for (int j = 0; j < clients[cnum].permissions_counter; j++)
            {
                if (strcmp(agents[i].address, clients[cnum].permissions[j]) == 0)
                {
                    p_exist = 1;

                    strcpy(msg, "s");
                    send(agents[i].socket, msg, sizeof(msg), 0);

                    n = sprintf(message, "Agent %s shutdown", agent_ip);
                    send(clients[cnum].socket, message, n, 0);

                    break;
                }
            }
            if (p_exist == 0)
            {
                n = sprintf(message, "No permissions to %s agent", agent_ip);
                send(clients[cnum].socket, message, n, 0);
            }
            break;
        }
    }
    if (a_exist == 0)
    {
        n = sprintf(message, "Agent %s not connected", agent_ip);
        send(clients[cnum].socket, message, n, 0);
    }

    memset(&agent_ip, 0, sizeof(agent_ip));
    memset(&message, 0, sizeof(message));
    memset(&msg, 0, sizeof(msg));

    return 0;
}

void *add(int cnum)
{
    int n;
    int a_exist = 0;
    char agent_ip[16];
    char message[50];
    char password[20];

    memset(&agent_ip, 0, sizeof(agent_ip));
    memset(&message, 0, sizeof(message));
    memset(&password, 0, sizeof(password));

    n = recv(clients[cnum].socket, agent_ip, sizeof(agent_ip), 0);
    printf("ADD agent ip %s - for %s : %d\n", agent_ip, clients[cnum].address, clients[cnum].socket);

    for (int i = 0; i < clients[cnum].permissions_counter; i++)
    {
        if (strcmp(agent_ip, clients[cnum].permissions[i]) == 0)
        {
            n = sprintf(message, "You already have %s agent privileges", agent_ip);
            send(clients[cnum].socket, message, n, 0);

            memset(&agent_ip, 0, sizeof(agent_ip));
            memset(&message, 0, sizeof(message));

            return 0;
        }
    }

    for (int i = 0; i < agents_count; i++)
    {
        if (strcmp(agent_ip, agents[i].address) == 0 && agents[i].connect == 1)
        {
            a_exist = 1;
            n = sprintf(message, "Password");
            send(clients[cnum].socket, message, n, 0);
            memset(&message, 0, sizeof(message));

            n = recv(clients[cnum].socket, password, 20, 0);
            // printf("ADD padd = %d : %s \n", n, password);

            if (strcmp(password, agents[i].pass) == 0)
            {
                n = sprintf(message, "Permission granted");
                send(clients[cnum].socket, message, n, 0);

                clients[cnum].permissions = (char **)realloc(clients[cnum].permissions, (clients[cnum].permissions_counter + 1) * sizeof(char *));
                clients[cnum].permissions[clients[cnum].permissions_counter] = (char *)malloc(sizeof(agent_ip) * sizeof(char));
                strcpy(clients[cnum].permissions[clients[cnum].permissions_counter], agent_ip);
                clients[cnum].permissions_counter++;
            }
            else
            {
                n = sprintf(message, "Bad password");
                send(clients[cnum].socket, message, n, 0);
            }
        }
    }

    if (a_exist == 0)
    {
        n = sprintf(message, "Agent %s not connected", agent_ip);
        send(clients[cnum].socket, message, n, 0);
    }

    memset(&agent_ip, 0, sizeof(agent_ip));
    memset(&message, 0, sizeof(message));
    memset(&password, 0, sizeof(password));

    return 0;
}

void *agents_thread(void *arg)
{
    int anum = *((int *)arg);
    int n;
    char msg[1];
    char password[20];

    memset(&password, 0, sizeof(password));

    n = recv(agents[anum].socket, password, 20, 0);
    agents[anum].pass = (char *)malloc(n * sizeof(char));
    strcpy(agents[anum].pass, password);
    // printf("AGENT pass = %d : %s : %s\n", n, password, agents[anum].pass);

    while (1)
    {
        n = recv(agents[anum].socket, msg, 1, 0);

        if (n == 0)
        {
            printf("Agent disconect - %s : %d\n", agents[anum].address, agents[anum].socket);
            agents[anum].connect = 0;
            break;
        }

        memset(&msg, 0, sizeof(msg));
    }

    memset(&password, 0, sizeof(password));

    pthread_exit(NULL);
}