#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
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
	int msqid, shmid, semid;
	
    key_t key = 10;
    key_t keyshm = 15;
    key_t keysem = 20;
    // Загружаем очередь
    if ((msqid = msgget(key, IPC_CREAT | 0777)) < 0)
    {
		perror ("Me: can not get queue");
    }

    // Создаем SM
    if ((shmid = shmget(keyshm, sizeof(message), IPC_CREAT | 0777)) < 0 ) 
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
    if ((semid = semget (keysem, 1, IPC_CREAT | 0777)) < 0) {
    	perror ("Me: can not get semaphore");
    }

	while (1) {
		// Получаем письмо
		msgrcv (msqid, (void *)&msgp, sizeof(msgp), 0, 0);
		
		printf("I got message = %s\n", msgp.mesg);

		// Проверяем семафорчик
		if (semctl(semid, 0, GETVAL, 0) == 0)
		{
			// Блокируем, пишем, разблокируем
			semctl (semid, 0, SETVAL, 1);
			memcpy (addr, &msgp, sizeof(msgp));
			printf ("I sent message = %s\n", msgp.mesg);
			semctl (semid, 0, SETVAL, 2);

			if (msgp.mtype == FINISH)
			{
				msgctl(msqid, IPC_RMID, NULL);
				break;
			}
		}
	}

	return 0;
}
