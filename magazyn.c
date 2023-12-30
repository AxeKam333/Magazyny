#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MAX_ZAM 1000
#define CZAS_PRACY 10

int gld = 0;
int surowce[3];
int x = 0;

// funkcja obslugujaca smierc kuriera i zakonczenie pracy magazynu
void smierc_kuriera(int signum)
{
    x++;
    if (x == 3)
    {
        printf("\nMagazyn (%i) konczy prace\n", getpid());
        printf("Zarobiono %i gld\n W magazynie zostalo:\n", gld);
        printf("%i surowca A\n", surowce[0]);
        printf("%i surowca B\n", surowce[1]);
        printf("%i surowca C\n", surowce[2]);
        
        exit(0);
    }
}

// otrzymywanie danych z dystrybutora
struct msgbuf
{
    long type;
    int nr_zam;
    int zamowienie[3];
};

// funkcjonalnosc kuriera
void kurier(int qid, int request, int response)
{
    struct msgbuf buf;
    int start = time(NULL);
    int koszt;
    while (1)
    {
        if (msgrcv(qid, &buf, (sizeof(buf) - sizeof(long)), 1, IPC_NOWAIT) != -1) // jesli przyjeto zamowienie
        {
            printf("Kurier (%i) otrzymal zamowienie %i: %i %i %i\n", getpid(), buf.nr_zam, buf.zamowienie[0], buf.zamowienie[1], buf.zamowienie[2]);
            char text[30];
            sprintf(text, "%i %i %i", buf.zamowienie[0], buf.zamowienie[1], buf.zamowienie[2]); // materialy dr. Wawrzyniaka
            write(request, text, 30);
            char text2[10];
            read(response, &text2, 10);
            printf("Kurier dostal odpowiedz: '%s'\n", text2);
            if (text2[0] == '\0')
            {
                printf("Kurier (%i) konczy prace - brak surowcow\n", getpid());
                return;
            }

            koszt = atoi(text2);
            printf("k: %i\n", koszt);

            start = time(NULL);
        }
        if (time(NULL) - start > CZAS_PRACY) // jesli uplynal czas pracy
        {
            printf("Kurier (%i) konczy prace\n", getpid());
            return;
        }
    }
}

int main(int argc, char *argv[])
{
    srand(time(NULL));

    if (argc != 3)
    {
        printf("Zla liczba argumentow\n");
        return 1;
    }

    // odczytywanie danych z argumentow
    // dodac walidacje
    char *adres_pliku = argv[1];
    int klucz = atoi(argv[2]);

    // wczytywanie z pliku
    int f = open(adres_pliku, O_RDONLY);
    if (f == -1)
    {
        perror("open");
        return 1;
    }
    close(0);
    dup(f);
    int ceny[3];
    for (int i = 0; i < 3; i++)
    {
        scanf("%i", &surowce[i]); //& - podaje adres zmiennej
        scanf("%i", &ceny[i]);
    }
    close(f);

    // tworzenie kolejki komunikatow do komunikacji z dystrybutorem
    int qid = msgget(klucz, IPC_CREAT | 0640);

    // pipe'y do komunikacji z kurierami
    // kurier wysyla request, magazyn odpowiada przez response
    int request[2], response[2];
    if (pipe(request) == -1 || pipe(response) == -1)
    {
        perror("pipe");
        return 1;
    }

    // tworzenie kurierow
    for (int i = 0; i < 3; i++)
    {
        if (fork() == 0)
        {
            close(request[0]);
            close(response[1]);
            kurier(qid, request[1], response[0]);
            return 0;
        }
    }

    // funkcjonalnosc magazynu
    close(request[1]);
    close(response[0]);
    signal(SIGCHLD, smierc_kuriera);

    int koszt;

    while (1)
    {
        char buf[30];
        if (read(request[0], &buf, 30) == 0) 
            break;
        printf("m: %s\n", buf);

        int zamowienie[3];
        zamowienie[0] = atoi(strtok(buf, " "));  // strtok zwraca pierwsza liczbe
        zamowienie[1] = atoi(strtok(NULL, " ")); // kontynuuje od ostatniego miejsca
        zamowienie[2] = atoi(strtok(NULL, " "));
        
            if (zamowienie[0] > surowce[0] || zamowienie[1] > surowce[1] || zamowienie[2] > surowce[2])
            {
                write(response[1], "", 1);
                continue;
            }
        
        koszt = 0;
        for (int i = 0; i < 3; i++)
        {
            surowce[i] -= zamowienie[i];
            koszt += zamowienie[i] * ceny[i];
        }
        gld += koszt;
        char koszt_str[10];
        sprintf(koszt_str, "%i", koszt);
        write(response[1], koszt_str, 10);
    }

    while (wait(NULL) > 0)
        ;
    return 0;
}