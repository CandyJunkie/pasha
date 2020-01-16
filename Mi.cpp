#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <string.h>
#include <string>
#include "calc_queue.h"


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

enum token_type {
    NUMBER,
    PLUS,
    MINUS,
    UNAR_MINUS,
    MULTIPLY,
    DIVIDE,
    POW,
    LBRACKET,
    RBRACKET // âñåãî 8
};

struct token {
    token_type m_type;
    float m_value;
};

void lex_analyze (const std::string expression, calc_queue<token> & result);
float calculate (calc_queue<token> & infix);

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

    while (1)
    {
        int flag = 0;
        if (semctl(semid, 0, GETVAL, 0) == 2)
        {
            // Блокируем, читаем
            semctl (semid, 0, SETVAL, 1);
            msgp = *addr;
            std::string expr = msgp.mesg;
            int result;

            try
            {
                // Cчитаем
                calc_queue<token> infix;
                lex_analyze (expr, infix);
                result = (int)calculate(infix);
            }
            catch (std::runtime_error& error)
            {
               printf("Плак-плак, деление на ноль\n");
               flag = 1;
            }
            // Пишем
            printf("result = %d\n", result);
            fflush(stdout);
            int len = strlen(msgp.mesg);
            expr = expr + ',' + ' ' + std::to_string(result) + std::string((flag > 0)? ", divided by zero" : ", ok");
            memcpy(msgp.mesg, expr.data(), strlen(expr.data()));
            msgp.mesg[strlen(expr.data())] = '\0';
            memcpy(addr, &msgp, sizeof(msgp));

            //Разблокируем
            semctl (semid, 0, SETVAL, 3);
            if (msgp.mtype == FINISH)
            {
                break;
            }
        }
    }
}