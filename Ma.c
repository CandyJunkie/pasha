#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

char generate_op()
{
	char ops[] = {'+', '-', '*', '/'};
	return ops[rand() % 4];
}

int input_num (char *str, int num)
{
	char numstr[4];
	sprintf(numstr, "%d", num);
	int i = 0;
	for (; num > 9; ++i)
	{
		str[i] = numstr[i];
		num /= 10;
	}
	str[i] = numstr[i];
	return i + 1;
}

void generate_arithm (char *str)
{
	int len = rand() % 3 + 2;
	int num = rand() % 100;
	char *pStr = str;
	int bytes = input_num(pStr, num);
	pStr += bytes;

	for (int i = 1; i < len; ++i)
	{
		char op = generate_op();
		*pStr = op;
		++pStr;
		num = rand() % 100;
		bytes = input_num(pStr, num);
		pStr += bytes;
	}
	*pStr = '\0';
}

enum mtypes
{
	EMPTY = 0,
    SEND,
    FINISH
};

typedef struct message
{
    long mtype;       // Тип сообщения
    char mesg[100];   // Само сообщение, длиной MSGSZ.
} message;

int main()
{
	int n = 2;
	message msgp;
	msgp.mtype = SEND;

    key_t key = 10;

    // Создаем очередь с правами на чтение и запись
    int msqid = msgget(key, IPC_CREAT | 0777);
    printf("Message queue created with id %d\n", msqid);

    int i = 0;
	while (1) {
		generate_arithm(msgp.mesg);
		printf("My message = %s\n", msgp.mesg);

		// Отправляем письмо и спим
		msgsnd (msqid, (const void *)&msgp, 100, 0);
		sleep(n);
		if (msgp.mtype == FINISH)
		{
			break;
		}
		if (i > 10) {
			msgp.mtype = FINISH;
		}
		++i;
	}

	return 0;
}
