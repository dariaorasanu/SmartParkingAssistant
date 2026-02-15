#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <cerrno>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <ctime>
#include "Parcare.h"

using namespace std;

#define SERVER_PORT 50000
#define BUF_SIZE 1024
#define INTERVAL_SCANARE 20
#define LOCURI_SCANATE 2
#define PROB_SCHIMBARE 20

int send_message(int sd, const char *msg)
{
    int len = strlen(msg);
    if (len > (BUF_SIZE - 1))
        return -1;

    int len_net = htonl(len);

    int n = send(sd, &len_net, sizeof(len_net), 0);
    if (n != (int)sizeof(len_net))
        return -1;

    n = send(sd, msg, len, 0);
    if (n != len)
        return -1;

    return 0;
}

int recv_message(int sd, char *buf, int buf_size)
{
    int len_net;
    int n = recv(sd, &len_net, sizeof(len_net), MSG_WAITALL);
    if (n <= 0)
        return n;

    int len = ntohl(len_net);
    if (len == 0)
    {
        if (buf_size > 0)
            buf[0] = '\0';
        return 0;
    }

    if (len >= buf_size)
        len = buf_size - 1;

    n = recv(sd, buf, len, MSG_WAITALL);
    if (n <= 0)
        return n;

    buf[n] = '\0';
    return n;
}

/*
        free_spots[i] = true  => liber in server
        free_spots[i] = false => ocupat in server
*/
bool parse_free_spots(const char *msg, bool free_spots[MAX_LOCURI + 1])
{
    // toate sunt ocupate
    for (int i = 1; i <= MAX_LOCURI; i++)
        free_spots[i] = false;

    // copie
    char tmp[BUF_SIZE];
    strncpy(tmp, msg, sizeof(tmp));
    tmp[sizeof(tmp) - 1] = '\0';

    char *token = strtok(tmp, " ");
    if (token == nullptr)
        return false;

    if (strcmp(token, "free_spots") != 0)
        return false;

    token = strtok(nullptr, " ");
    if (token == nullptr)
    {
        return false;
    }

    if (strcmp(token, "niciunul") == 0)
    {
        return true; // raman toate false
    }

    while (token != nullptr)
    {
        int id = atoi(token);
        if (id >= 1 && id <= MAX_LOCURI)
        {
            free_spots[id] = true;
        }
        token = strtok(nullptr, " ");
    }

    return true;
}

int main()
{
    srand(time(NULL));

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        cout << "Eroare la socket\n";
        return 1;
    }

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        close(sock);
        return 1;
    }

    send_message(sock, "connect");
    char buf[BUF_SIZE];
    recv_message(sock, buf, BUF_SIZE);

    cout << "Camera conectata. Scanez " << LOCURI_SCANATE << " locuri la fiecare " << INTERVAL_SCANARE << " sec.\n";
    sleep(INTERVAL_SCANARE);

    while (true)
    {
        // cerem lista locurilor libere
        if (send_message(sock, "list_free") < 0)
            break;

        int r = recv_message(sock, buf, BUF_SIZE);
        if (r <= 0)
            break;

        // construim vectorul "caracteristic" pentru fiecare loc daca e liber sau nu
        bool server_free[MAX_LOCURI + 1];
        if (parse_free_spots(buf, server_free) == false)
        {
            cout << "Camera: raspuns list_free neasteptat: " << buf << "\n";
            sleep(INTERVAL_SCANARE);
            continue;
        }

        // ne folosim de used ca sa nu alegem acelasi loc de 2 ori
        bool used[MAX_LOCURI + 1];
        for (int i = 1; i <= MAX_LOCURI; i++)
            used[i] = false;

        for (int k = 0; k < LOCURI_SCANATE; k++)
        {
            // aleg un id random
            int id;
            do
            {
                id = 1 + (rand() % MAX_LOCURI);
            } while (used[id] == true);
            used[id] = true;

            // ce vede serverul
            bool server = server_free[id];

            // ce vede camera (momentan ca la server)
            bool camera = server;

            int chance = rand() % 100;
            if (chance < PROB_SCHIMBARE)
            {
                // inversam ce vede camera
                if (camera == true)
                    camera = false;
                else
                    camera = true;
            }
            // construim textul liber / ocupat
            const char *stare;
            if (camera == true)
                stare = "liber";
            else
                stare = "ocupat";

            if (camera == server)
            {
                cout << "Camera a confirmat ca locul " << id << " a ramas " << stare << "." << "\n";
            }
            else
            {

                char cmd[64];
                snprintf(cmd, sizeof(cmd), "update %d %s", id, stare);

                cout << "Camera a detectat ca locul " << id << " a devenit " << stare << "." << "\n";
                cout << "Camera -> " << cmd << "\n";
                if (send_message(sock, cmd) < 0)
                    break;

                int reply = recv_message(sock, buf, BUF_SIZE);
                if (reply <= 0)
                    break;

                cout << "Server <- " << buf << "\n";

                usleep(200000);
            }
        }

        sleep(INTERVAL_SCANARE);
    }

    close(sock);
    cout << "Camera inchisa.\n";
    return 0;
}
