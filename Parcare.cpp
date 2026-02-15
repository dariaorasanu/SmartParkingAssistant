#include <iostream>
#include "Parcare.h"
#include <cstring>
#include <cstdio>

using namespace std;

Parcare::Parcare()
{
    init();
}

void Parcare::init()
{
    // toate locurile sunt libere
    for (int id = 1; id <= MAX_LOCURI; id++)
    {
        locuri[id] = 0;
    }
}
bool Parcare::isValidId(int id)
{
    if (id >= 1 && id <= MAX_LOCURI)
        return true;
    else
        return false;
}

bool Parcare::isFree(int id)
{
    if (!isValidId(id))
    {
        return false;
    }
    if (locuri[id] == 0)
        return true;
    return false;
}
int Parcare::get_locuri_libere(char *buf, int buf_size)
{
    if (buf == nullptr || buf_size <= 0)
    {
        return 0;
    }

    buf[0] = '\0';

    // prefixul protocolului
    snprintf(buf, buf_size, "free_spots");

    int count = 0;

    for (int id = 1; id <= MAX_LOCURI; id++)
    {
        if (locuri[id] == 0)
        {
            // loc liber
            count++;
            char tmp[32];
            snprintf(tmp, sizeof(tmp), " %d", id);
            strncat(buf, tmp, buf_size - (int)strlen(buf) - 1);
        }
    }

    if (count == 0)
    {
        strncat(buf, " niciunul", buf_size - (int)strlen(buf) - 1);
    }

    return count;
}
const char *Parcare::update_loc(int id, const char *stare)
{
    if (!isValidId(id))
    {
        return "id_invalid";
    }
    if (strcmp(stare, "liber") == 0)
    {
        locuri[id] = 0;
    }
    else if (strcmp(stare, "ocupat") == 0)
    {
        locuri[id] = 1;
    }
    else
    {
        return "stare_invalida";
    }

    return nullptr;
}
const char *Parcare::rezerva_loc(int id)
{
    if (!isValidId(id))
    {
        return "id_invalid";
    }
    if (locuri[id] == 1)
    {
        return "deja_rezervat";
    }

    locuri[id] = 1;
    return nullptr;
}