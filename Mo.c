#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <string.h>

typedef struct message{
    long mtype;       // Тип сообщения
    char mesg[100];   // Само сообщение, длиной MSGSZ.
} message;

enum mtypes
{
    EMPTY = 0,
    SEND,
    FINISH
};

int main()
{
    message msgp;
    int shmid, semid;
    
    key_t keyshm = 15;
    key_t keysem = 20;


    // Создаем SM
    if ((shmid = shmget(keyshm, sizeof(message), 0777)) < 0 ) 
    {
        perror("Me: can not get sharedmem");
        return 1;
    }

    message *addr;
    if ((addr = (message *)(shmat(shmid, 0, 0))) == NULL)
    {
        perror("Shared memory attach error");
    }

    // Создаем семафорчик
    if ((semid = semget (keysem, 1, 0777)) < 0) {
        perror ("Me: can not get semaphore");
    }

    FILE *fo = fopen("Hello.txt", "wb+");

    while (1)
    {
        if (semctl(semid, 0, GETVAL, 0) == 3)
        {
            // Блокируем, читаем
            semctl (semid, 0, SETVAL, 1);
            msgp = *addr;

            // Пишем
            fprintf(fo, "%s\n", msgp.mesg);
            printf("%s\n", msgp.mesg);

            //Разблокируем
            semctl (semid, 0, SETVAL, 0);
            if (msgp.mtype == FINISH)
            {
            	semctl (semid, 0, IPC_RMID, 0);
            	shmctl (shmid, IPC_RMID , NULL);
            	break;
            }
        }
    }

    fclose(fo);
}