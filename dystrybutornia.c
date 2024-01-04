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
#define FREQ 0.5

struct msgbuf
{
    long type;
    int nr_zam;
    int zamowienie[3];
};

void licz_gld(int *gld, int fifo)
{
    int ile;
    if (read(fifo, &ile, sizeof(int)) > 0)
    {
        *gld += ile;
        printf("Stan: %i gld\n", *gld);
    }
    else
    {
        printf("---\nDystrybutornia konczy prace\nWydano lacznie %i gld\n", *gld);
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    srand(time(NULL));

    if (argc != 6)
    {
        printf("Zla liczba argumentow\n");
        return 1;
    }

    // odczytywanie danych z argumentow
    char* klucz_fifo = argv[1];
    int klucz_msg = atoi(argv[1]);
    if (klucz_msg <= 0)
    {
        printf("!!!\nNiepoprawny klucz\n");
        return 1;
    }
    int liczba_zamowien = atoi(argv[2]);
    if (liczba_zamowien > MAX_ZAM || liczba_zamowien <= 0)
    {
        printf("!!!\nNiepoprawna liczba zamowien\n");
        liczba_zamowien = MAX_ZAM/2;
    }
    int max[3] = {atoi(argv[3]), atoi(argv[4]), atoi(argv[5])};
    for (int i = 0; i < 3; i++)
    {
        if (max[i] <= 0 || max[i] > 10000)
        {
            printf("!!!\nNiepoprawna maksymalna liczba produktow\n");
            max[i] = 100;
        }
    }
    printf("%i %i %i %i %i\n", klucz_msg, liczba_zamowien, max[0], max[1], max[2]);

    int wydany_gld = 0; 

    mkfifo(klucz_fifo, 0640);
    int fifo = open(klucz_fifo, O_RDONLY);

    int qid = msgget(klucz_msg, IPC_CREAT | 0640);
    struct msgbuf buf;
    // sleep(FREQ);

    for (int i = 0; i < liczba_zamowien; i++)
    {

        buf.type = 1;
        buf.nr_zam = i;
        for (int j = 0; j < 3; j++)
        {
            buf.zamowienie[j] = rand() % (max[j] + 1);
        }
        // wysylanie zamowienia
        printf("Zamowienie nr %i: %i %i %i\n", buf.nr_zam, buf.zamowienie[0], buf.zamowienie[1], buf.zamowienie[2]);
        msgsnd(qid, &buf, (sizeof(buf) - sizeof(long)), 0);
        sleep(FREQ);
        licz_gld(&wydany_gld, fifo);
    }

    printf("---\nDystrybutornia wyslala wszystkie zamowienia\n");
    printf("Wydano lacznie %i gld\n", wydany_gld);

    if (msgctl(qid, IPC_RMID, NULL) == -1)
    { // usuwanie kolejki
        perror("msgctl");
        return 1;
    }
    close(fifo);
    unlink(klucz_fifo);

    return 0;
}