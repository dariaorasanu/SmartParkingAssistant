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
#include <sys/select.h>

using namespace std;

#define SERVER_PORT 50000
#define BUF_SIZE 1024

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
    int len_net = 0;
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

void afiseaza_meniu()
{
    cout << "=== Client SmartParking ===\n";
    cout << "1. connect\n";
    cout << "2. list_free\n";
    cout << "3. reserve\n";
    cout << "4. watch\n";
    cout << "5. recommend\n";
    cout << "6. disconnect\n";
    cout << "7. exit client\n";
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
    int sock = -1;
    bool conectat = false;
    double x = 0.0, y = 0.0;
    bool am_pozitie = false;
    char buf[BUF_SIZE];
    int loc_rezervat = -1; // ultimul loc rezervat

    afiseaza_meniu();

    while (true)
    {
        cout << "\nOptiune (1-7): ";
        int opt;
        if (!(cin >> opt))
        {
            cout << "Input invalid.\n";
            cin.clear();
            cin.ignore(10000, '\n');
            continue;
        }

        if (opt == 1)
        {
            // CONNECT
            if (conectat)
            {
                cout << "Deja conectat.\n";
                continue;
            }

            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0)
            {
                cout << "Eroare la socket" << endl;
                exit(1);
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
                sock = -1;
                continue;
            }

            if (send_message(sock, "connect") < 0)
            {
                cout << "Eroare la trimiterea comenzii connect.\n";
                close(sock);
                sock = -1;
                continue;
            }

            int r = recv_message(sock, buf, BUF_SIZE);
            if (r <= 0)
            {
                cout << "Eroare la raspunsul connect.\n";
                close(sock);
                sock = -1;
                continue;
            }
            conectat = true;
            cout << "S-a efectuat conectarea la server.\n";
            if (am_pozitie == false)
            {
                if (sscanf(buf, "connect_ok %lf %lf", &x, &y) == 2)
                {
                    am_pozitie = true;
                    cout << "Pozitia ta este: (" << x << ", " << y << ")\n";
                }
                else
                {
                    am_pozitie = false;
                    cout << "Nu am primit coordonate.\n";
                }
            }
            else
            {
                cout << "Pozitia ta este: (" << x << ", " << y << ")\n";
            }
        }
        else if (opt == 2)
        {
            // LIST_FREE
            if (!conectat)
            {
                cout << "Nu esti conectat.\n";
                continue;
            }

            if (send_message(sock, "list_free") < 0)
            {
                cout << "Eroare la list_free.\n";
                break;
            }

            int r = recv_message(sock, buf, BUF_SIZE);
            if (r <= 0)
            {
                cout << "Eroare la raspuns list_free.\n";
                break;
            }

            if (strncmp(buf, "free_spots", 10) == 0)
            {
                char *p = buf + 11; // sarim peste "free_spots "
                cout << "Locurile libere sunt: " << p << "\n";
            }
            else
            {
                // daca serverul trimite altceva
                cout << "Server: " << buf << "\n";
            }
        }
        else if (opt == 3)
        {
            // RESERVE
            if (!conectat)
            {
                cout << "Nu esti conectat.\n";
                continue;
            }

            int id;
            cout << "Id loc parcare: ";
            if (!(cin >> id))
            {
                cout << "Id invalid.\n";
                cin.clear();
                cin.ignore(10000, '\n');
                continue;
            }

            char cmd[64];
            snprintf(cmd, sizeof(cmd), "reserve %d", id);

            if (send_message(sock, cmd) < 0)
            {
                cout << "Eroare la reserve.\n";
                break;
            }

            int r = recv_message(sock, buf, BUF_SIZE);
            if (r <= 0)
            {
                cout << "Eroare la raspuns reserve.\n";
                break;
            }
            if (strncmp(buf, "reserve_ok", 10) == 0)
            {
                int id_loc;
                if (sscanf(buf, "reserve_ok %d", &id_loc) == 1)
                {
                    if (loc_rezervat != -1 && loc_rezervat != id_loc)
                    {
                        char cmd2[64];
                        snprintf(cmd2, sizeof(cmd2), "update %d liber", loc_rezervat);

                        send_message(sock, cmd2);
                        recv_message(sock, buf, BUF_SIZE);
                    }

                    loc_rezervat = id_loc;

                    double x1, y1;
                    coordonate_id(id_loc, x1, y1);
                    x = x1;
                    y = y1;

                    cout << "Ati rezervat cu succes locul " << id_loc << ".\n";
                    cout << "Pozitia ta a fost actualizata la: (" << x << ", " << y << ")\n";
                }
                else
                {
                    cout << "Raspuns server invalid.\n";
                }
            }
            else
            {
                cout << "Rezervare esuata: " << buf << ".\n";
            }
        }
        else if (opt == 4)
        {
            if (!conectat)
            {
                cout << "Nu esti conectat.\n";
                continue;
            }

            int id;
            cout << "Ce loc vrei sa urmaresti? (1-10): ";
            if (!(cin >> id))
            {
                cout << "Input invalid.\n";
                cin.clear();
                cin.ignore(10000, '\n');
                continue;
            }

            // consumam newline ramas dupa cin >> id,
            cin.ignore(10000, '\n');

            char cmd[64];
            snprintf(cmd, sizeof(cmd), "watch %d", id);

            if (send_message(sock, cmd) < 0)
            {
                cout << "Eroare la watch.\n";
                break;
            }

            // asteptam raspunsul la comanda watch
            int r = recv_message(sock, buf, BUF_SIZE);
            if (r <= 0)
            {
                cout << "Eroare la raspuns watch.\n";
                break;
            }
            cout << "Server: " << buf << "\n";

            // daca serverul a confirmat, intram in watch-mode
            if (strncmp(buf, "watch_ok", 8) == 0)
            {
                cout << "Ascult alerte pentru locul " << id
                     << ". Apasa ENTER ca sa iesi din modul watch.\n";

                while (true)
                {
                    fd_set set;
                    FD_ZERO(&set);
                    FD_SET(sock, &set); // server
                    FD_SET(0, &set);    // tastatura

                    int ready = select(sock + 1, &set, NULL, NULL, NULL);
                    if (ready < 0)
                    {
                        perror("select");
                        break;
                    }

                    // ENTER -> iesim din watch-mode
                    if (FD_ISSET(0, &set))
                    {
                        char tmp[8];
                        // golesc ce a tastat utilizatorul
                        read(0, tmp, sizeof(tmp));
                        cout << "Ies din watch.\n";
                        break;
                    }

                    // mesaj de la server
                    if (FD_ISSET(sock, &set))
                    {
                        int r = recv_message(sock, buf, BUF_SIZE);
                        if (r <= 0)
                        {
                            cout << "Conexiune inchisa.\n";
                            break;
                        }

                        if (strncmp(buf, "alert ", 6) == 0)
                        {
                            cout << "[NOTIFICARE] " << buf << "\n";
                        }
                        else
                        {
                            cout << "Server: " << buf << "\n";
                        }
                    }
                }
            }
        }
        else if (opt == 5)
        {
            // REOMANDAREA CELUI MAI APROPIAT LOC
            if (!conectat)
            {
                cout << "Nu esti conectat.\n";
                continue;
            }

            // userul vede doar "recommend", dar noi trimitem coordonatele lui
            char cmd[128];
            snprintf(cmd, sizeof(cmd), "recommend %.2f %.2f", x, y);

            if (send_message(sock, cmd) < 0)
            {
                cout << "Eroare la recommend.\n";
                break;
            }

            // primim raspuns
            while (true)
            {
                int r = recv_message(sock, buf, BUF_SIZE);
                if (r <= 0)
                {
                    cout << "Eroare la raspuns recommend.\n";
                    break;
                }

                if (strncmp(buf, "recommend_ok", 12) == 0)
                {
                    int id;
                    double rx, ry, dist;
                    if (sscanf(buf, "recommend_ok %d", &id) == 1)
                    {
                        cout << "Recomandare: locul " << id << "." << "\n";
                    }
                    else
                    {
                        cout << "Raspuns recommend invalid: " << buf << "\n";
                    }
                }
                else
                {
                    cout << "Server: " << buf << "\n";
                }
                break;
            }
        }

        else if (opt == 6)
        {
            // DECONCTARE DE LA SERVER
            if (!conectat)
            {
                cout << "Nu esti conectat.\n";
                continue;
            }

            if (send_message(sock, "disconnect") < 0)
            {
                cout << "Eroare la disconnect.\n";
                break;
            }

            int r = recv_message(sock, buf, BUF_SIZE);
            if (r <= 0)
            {
                cout << "Eroare la raspuns reserve.\n";
                break;
            }

            close(sock);
            sock = -1;
            conectat = false;
            cout << "Te-ai deconectat de la server.\n";
        }
        else if (opt == 7)
        {
            // QUIT
            if (conectat && sock >= 0)
            {
                close(sock);
            }
            cout << "Iesire client.\n";
            break;
        }
        else
        {
            cout << "Optiune necunoscuta.\n";
        }
    }

    if (conectat && sock >= 0)
    {
        close(sock);
    }

    return 0;
}
