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
#include "Parcare.h"
#include <ctime>
#include <cmath>
#include <sys/select.h>

using namespace std;

#define SERVER_PORT 50000
#define BUF_SIZE 1024
#define MAX_FDS 1024
int watch_spot[MAX_FDS];

int send_message(int sd, const char *msg)
{
    int len = strlen(msg);
    if (len > (BUF_SIZE - 1))
    {
        return -1;
    }
    int len_net = htonl(len);

    int n = send(sd, &len_net, sizeof(len_net), 0);
    if (n != sizeof(len_net))
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
ce coordonate vreau sa pun:
1: (0,0)  3: (1,0)  5: (2,0)  7: (3,0)  9: (4,0)
2: (0,1)  4: (1,1)  6: (2,1)  8: (3,1)  10: (4,1)
*/
void coordonate_id(int id, double &id_x, double &id_y)
{
    if (id % 2 == 1)
    {                        // impar = sus
        id_x = (id - 1) / 2; // 1->0, 3->1, 5->2, 7->3, 9->4
        id_y = 0.0;
    }
    else
    {                        // par = jos
        id_x = (id - 2) / 2; // 2->0, 4->1, 6->2, 8->3, 10->4
        id_y = 1.0;
    }
}
int main()
{
    srand(time(NULL));
    for (int i = 0; i < MAX_FDS; i++)
        watch_spot[i] = -1;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind");
        close(server_fd);
        exit(1);
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("listen");
        close(server_fd);
        exit(1);
    }

    cout << "Server pornit pe portul " << SERVER_PORT << "\n";

    fd_set master_set, read_set;
    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);
    int fd_max = server_fd;

    Parcare parcare;
    while (true)
    {
        read_set = master_set;

        int activity = select(fd_max + 1, &read_set, NULL, NULL, NULL);
        if (activity < 0)
        {
            perror("select");
            continue;
        }

        // conexiuni noi
        if (FD_ISSET(server_fd, &read_set))
        {
            struct sockaddr_in cli_addr;
            socklen_t cli_len = sizeof(cli_addr);
            int client_fd = accept(server_fd, (struct sockaddr *)&cli_addr, &cli_len);
            if (client_fd >= 0)
            {
                FD_SET(client_fd, &master_set);
                if (client_fd < MAX_FDS)
                {
                    watch_spot[client_fd] = -1;
                }
                if (client_fd > fd_max)
                {
                    fd_max = client_fd;
                }
            }
        }

        // date de la clientii existenti
        for (int fd = 0; fd <= fd_max; fd++)
        {
            if (fd == server_fd)
                continue;
            if (FD_ISSET(fd, &read_set))
            {
                char buf[BUF_SIZE];
                int r = recv_message(fd, buf, BUF_SIZE);

                if (r <= 0)
                {
                    close(fd);
                    FD_CLR(fd, &master_set);
                    if (fd < MAX_FDS)
                    {
                        watch_spot[fd] = -1; // nu mai urmareste nimic
                    }
                }
                else
                {
                    cout << "Comanda primita: " << buf << "\n";

                    char reply[BUF_SIZE];

                    if (strcmp(buf, "connect") == 0)
                    {
                        double x, y;

                        int chance = rand() % 100;

                        if (chance < 30)
                        {
                            // "la intrare" (stanga, mijloc)
                            x = 0.0;
                            y = 0.5;
                        }
                        else
                        {
                            // random in interior
                            x = ((double)rand() / RAND_MAX) * 4.0; // 0..4
                            y = ((double)rand() / RAND_MAX) * 1.0; // 0..1
                        }

                        snprintf(reply, sizeof(reply), "connect_ok %.2f %.2f", x, y);
                        send_message(fd, reply);
                    }

                    else if (strcmp(buf, "list_free") == 0)
                    {
                        int nr = parcare.get_locuri_libere(reply, sizeof(reply));
                        cout << "Sunt " << nr << " locuri libere in parcare\n";
                        send_message(fd, reply);
                    }
                    else if (strncmp(buf, "reserve", 7) == 0)
                    {
                        int id;

                        if (sscanf(buf, "reserve %d", &id) == 1)
                        {
                            const char *raspuns = parcare.rezerva_loc(id);

                            if (raspuns == nullptr)
                            {
                                snprintf(reply, sizeof(reply), "reserve_ok %d", id);
                            }
                            else
                            {
                                // raspuns este "id_invalid" sau "deja_rezervat"
                                snprintf(reply, sizeof(reply), "error %s", raspuns);
                            }
                        }
                        else
                        {
                            snprintf(reply, sizeof(reply), "error format_invalid");
                        }

                        send_message(fd, reply);
                    }

                    else if (strncmp(buf, "update", 6) == 0)
                    {
                        int id;
                        char stare[32];

                        if (sscanf(buf, "update %d %s", &id, stare) == 2)
                        {
                            bool wasFree = parcare.isFree(id);

                            const char *raspuns = parcare.update_loc(id, stare);

                            if (raspuns == nullptr)
                            {
                                bool nowFree = parcare.isFree(id);

                                snprintf(reply, sizeof(reply), "update_ok");
                                send_message(fd, reply);

                                // daca locul a devenit liber -> notificam clientii care il urmaresc
                                if (wasFree == false && nowFree == true)
                                {
                                    for (int i = 0; i <= fd_max; i++)
                                    {
                                        if (i == server_fd)
                                            continue;

                                        if (i < MAX_FDS && watch_spot[i] == id)
                                        {
                                            char buf1[64];
                                            snprintf(buf1, sizeof(buf1), "alert free %d", id);
                                            send_message(i, buf1);
                                        }
                                    }
                                }
                            }
                            else
                            {
                                snprintf(reply, sizeof(reply), "error %s", raspuns);
                                send_message(fd, reply);
                            }
                        }
                        else
                        {
                            snprintf(reply, sizeof(reply), "error format_invalid");
                            send_message(fd, reply);
                        }
                    }

                    else if (strcmp(buf, "disconnect") == 0)
                    {

                        snprintf(reply, sizeof(reply), "disconnect_ok");
                        send_message(fd, reply);
                        close(fd);
                        FD_CLR(fd, &master_set);
                        if (fd < MAX_FDS)
                        {
                            watch_spot[fd] = -1; // nu mai urmareste nimic
                        }
                    }
                    else if (strncmp(buf, "watch", 5) == 0)
                    {
                        int id;
                        if (sscanf(buf, "watch %d", &id) == 1)
                        {
                            if (id < 1 || id > MAX_LOCURI)
                            {
                                snprintf(reply, sizeof(reply), "error id_invalid");
                            }
                            else if (parcare.isFree(id) == true)
                            {
                                snprintf(reply, sizeof(reply), "error deja_liber");
                            }
                            else
                            {
                                if (fd < MAX_FDS)
                                    watch_spot[fd] = id;
                                snprintf(reply, sizeof(reply), "watch_ok %d", id);
                            }
                        }
                        else
                        {
                            snprintf(reply, sizeof(reply), "error format_invalid");
                        }

                        send_message(fd, reply);
                    }
                    else if (strncmp(buf, "recommend", 9) == 0)
                    {
                        double ux, uy; // coordonatele userului
                        if (sscanf(buf, "recommend %lf %lf", &ux, &uy) == 2)
                        {
                            int best_id = -1;
                            double best_x = 0.0, best_y = 0.0;
                            double best_dist = 1e18;

                            for (int id = 1; id <= MAX_LOCURI; id++)
                            {
                                if (parcare.isFree(id) == true)
                                {
                                    double sx, sy;
                                    coordonate_id(id, sx, sy);

                                    double dx = sx - ux;
                                    double dy = sy - uy;
                                    double d = sqrt(dx * dx + dy * dy);

                                    if (d < best_dist)
                                    {
                                        best_dist = d;
                                        best_id = id;
                                        best_x = sx;
                                        best_y = sy;
                                    }
                                }
                            }

                            if (best_id == -1)
                            {
                                snprintf(reply, sizeof(reply), "error no_free_spots");
                            }
                            else
                            {
                                snprintf(reply, sizeof(reply), "recommend_ok %d", best_id);
                            }
                        }
                        else
                        {
                            snprintf(reply, sizeof(reply), "error format_invalid");
                        }

                        send_message(fd, reply);
                    }

                    else
                    {
                        snprintf(reply, sizeof(reply), "error unknown_command");
                        send_message(fd, reply);
                    }
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
