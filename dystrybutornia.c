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

struct msgbuf
{
    long type;
    int nr_zam;
    int zamowienie[3];
};

int main(int argc, char *argv[])
{
    srand(time(NULL));

    if (argc != 6)
    {
        printf("Zla liczba argumentow\n");
        return 1;
    }

    // odczytywanie danych z argumentow
    // dodac walidacje

    int klucz = atoi(argv[1]);
    int liczba_zamowien = atoi(argv[2]);
    if (liczba_zamowien > MAX_ZAM)
    {
        printf("Zbyt wiele zamowien\n");
        liczba_zamowien = MAX_ZAM;
    }
    else if (liczba_zamowien < 0)
    {
        printf("Liczba zamowien nie moze byc ujemna\n");
        liczba_zamowien = 0;
    }
    int max[3] = {atoi(argv[3]), atoi(argv[4]), atoi(argv[5])};
    printf("%i %i %i %i %i\n", klucz, liczba_zamowien, max[0], max[1], max[2]);
    for (int i = 0; i < 3; i++)
    {
        if (max[i] < 0)
        {
            printf("Liczba produktow nie moze byc ujemna\n");
            max[i] = 0;
        }
    }

    int wydany_gld = 0; //!!!!!!!!!!!

    // mkfifo(klucz, 0640);
    // int kolejka = open(klucz, O_WRONLY);

    int qid = msgget(klucz, IPC_CREAT | 0640);
    struct msgbuf buf;

    for (int i = 0; i < liczba_zamowien; i++)
    {
        sleep(1);

        buf.type = 1;
        buf.nr_zam = i;
        for (int j = 0; j < 3; j++)
        {
            buf.zamowienie[j] = rand() % (max[j] + 1);
        }
        // wysylanie zamowienia
        printf("Zamowienie nr %i: %i %i %i\n", buf.nr_zam, buf.zamowienie[0], buf.zamowienie[1], buf.zamowienie[2]);
        msgsnd(qid, &buf, (sizeof(buf) - sizeof(long)), 0);
    }

    if (msgctl(qid, IPC_RMID, NULL) == -1) { //usuwanie kolejki
        perror("msgctl");
        return 1;
    }

    return 0;
}