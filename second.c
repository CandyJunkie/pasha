#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

void gen_task (char *task)
{
	task[0] = '1' + rand() % 9;
	task[1] = ' ';
	switch (rand() % 5)
	{
		case 0:
		{
			char tmp[] = " Помыть посуду";
			for (int i = 0; tmp[i] != '\0'; ++i)
			{
				task[i + 2] = tmp[i];
			}
			break;
		}
		case 1:
		{
			char tmp[] = " Помыть пол";
			for (int i = 0; tmp[i] != '\0'; ++i)
			{
				task[i + 2] = tmp[i];
			}
			break;
		}
		case 2:
		{
			char tmp[] = " Помыть машину";
			for (int i = 0; tmp[i] != '\0'; ++i)
			{
				task[i + 2] = tmp[i];
			}
			break;
		}
		case 3:
		{
			char tmp[] = " Помыть голову";
			for (int i = 1; tmp[i - 1] != '\0'; ++i)
			{
				task[i + 1] = tmp[i];
			}
			break;
		}
		case 4:
		{
			char tmp[] = " Помыть овощи";
			for (int i = 0; tmp[i] != '\0'; ++i)
			{
				task[i + 2] = tmp[i];
			}
			break;
		}
	}
}

void my_handler(int sig)
{
    if (sig == SIGUSR1)
    {
        printf("SIGUSR1 received, ppid = %d\n", getpid());
        fflush(stdout);
        _exit(1);
    }
}

int main(int argc, char **argv)
{
    /*/
    if (argc < 3) {
    	printf ("Ivalid argc. Enter 2 args: process_num and task_num");
    	_exit(1);
    }
    //*/

	// Проверяем, надо ли делать аборт
	signal(SIGUSR1, my_handler);

    char tasks[100][100];
    int bubble = 0;

    int p = atoi("3"); // количество процессов
    int ntask = atoi("8"); // количество задач
    char readbuffer[150];

    for (int i = 0; i < ntask; ++i)
    {
    	gen_task(tasks[i]);
    }

    // Пайпы
    int fdProcess[p][2];
    //Процессы детей
    pid_t childpid[p];

    for(int i = 0; i < p; i++) {
        // Создаем пайп для ребенка
        pipe(fdProcess[i]);
        // Порождаем ребенка
        switch ((childpid[i] = fork()))
        {
        	case -1: // error
        	{
        		perror("fork");
            	_exit(1);
        	}
        	case 0: // child
        	{
        		bubble = i + 1;
        		break;
        	}
        	default: // parent
        	{
        		printf("PARENT: %d, CHILD: %d\n", getpid(), childpid[i]);
        		fflush(stdout);
        	}
        }   
    }

    struct pollfd fds[p][1];

    // Количество задач, которые выдаем без получения отчета о завершении прошлой задачи
    int first_tasks = (p < ntask)? p : ntask;

    while(1) {
    	if(bubble > 0) { // Если ребенок, значит пора работать
            //Считываем задание
            read(fdProcess[bubble - 1][1], readbuffer, sizeof(readbuffer));

            //В полученной строке первое число - количество секунд
            int sleepTime;
            sscanf(readbuffer, "%d", &sleepTime);
            sleep(sleepTime);

            printf ("%s - задание сделано by %d\n", readbuffer, getpid());
            fflush(stdout);

            //Отправим ответ родителю, что задание сделано
            write(fdProcess[bubble - 1][1], readbuffer, (strlen(readbuffer)+1));


        }
        else // Если родитель, то пора раздавать и принимать работу
        {
        	for(int i = 0; i < p; i++) {
	            fds[i][0].fd = fdProcess[i][0];
	            fds[i][0].events = POLLIN;

	            // Сначала выдаем задачи просто так
	            if(first_tasks > 0) {
	                write(fdProcess[i][1], tasks[ntask-1], (strlen(tasks[ntask-1])+1));
	                printf("PARENT: выдал задание ребёнку pid: %d\n", childpid[i]);
	                fflush(stdout);

	                first_tasks--;
	                ntask--;
	            }
	            // А потом после завершения предыдущей
	            else {
	                if(poll(fds[i], 1, 1000) > 0) {

	                    // Ребёнок присылает задание, которое он сделал
	                    read(fdProcess[i][0], readbuffer, sizeof(readbuffer));
	                    printf("PARENT: Received string: %s\n", readbuffer);
	                    fflush(stdout);

	                    // Задания закончились - начинаем убивать детей
	                    if(ntask <= 0) {
	                    	if (childpid[i] > 0) 
	                    	{
	                    		kill(childpid[i], SIGUSR1);
		                        close(fdProcess[i][0]);
		                        close(fdProcess[i][1]);
		                        
		                        ntask--;
		                        printf ("PARENT: Убил ребенка: %d, МУАХАХА.\n", childpid[i]);
		                        childpid[i] = 0;
		                        //Убили всех детей
			                    if(ntask == -p) {
			                        exit(0);
			                    }
	                    	}  
	                    }
	                    //Выдаём следующее задание
	                    else {
	                        write(fdProcess[i][1], tasks[ntask-1], (strlen(tasks[ntask-1])+1));
	                        printf("PARENT: выдал задание ребёнку pid: %d\n", childpid[i]);
	                        fflush(stdout);
	                        ntask--;
	                    }
	                }
	            }
        	}

        }
        
    }
    return 0;
}