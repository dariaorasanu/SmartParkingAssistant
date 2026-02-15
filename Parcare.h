#pragma once
#define MAX_LOCURI 10

class Parcare {
private:
    // 1 = ocupat, 0 = liber
    int locuri[MAX_LOCURI + 1];

    bool isValidId(int id);

public:
    Parcare();
    void init();
    //returneaza cate locuri libere a gasit si pune pozitiile locurilor libere intr-un buffer
    int get_locuri_libere(char *buf, int buf_size);
    
    const char* rezerva_loc(int id);
    // încearcă să rezerve locul "id"
    // returnează nullptr daca e ok
    // mesajul de eroare daca nu
    

    
    const char* update_loc(int id, const char *stare);
    // update de la senzor: stare = "liber" sau "ocupat"
    // ret nullptr daca e ok
    //mesaj de eroare daca nu

    bool isFree(int id);
};
