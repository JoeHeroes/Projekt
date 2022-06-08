#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <syslog.h>

int chairs;
int currentclientid = 0;


pthread_mutex_t queuechairs = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t accessBarber = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gettingCut = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t clientsAvailable = PTHREAD_COND_INITIALIZER;
pthread_cond_t barberReady = PTHREAD_COND_INITIALIZER;
pthread_cond_t barberCutting = PTHREAD_COND_INITIALIZER;

int waitingclients = 0; //liczba czekajacych klientow
int totalclients = 0; //liczba wszystkich klientow ktorzy byli w salonie
int resignedclients = 0; //liczba klientow ktorzy zrezygnowali



typedef struct Client_Struct
{
	int id;
	pthread_cond_t turn;
	struct Client_Struct *next;
} Client;

Client *barberQueue = NULL;
Client *resignedClients = NULL;

Client *addClientToQueue(Client **start, int id)
{
	Client *new = malloc(sizeof(Client));
	new->id = id;
	new->next = NULL;
	pthread_cond_init(&new->turn, NULL);

	if (*start == NULL)
	{
		(*start) = new;
	}
	else
	{
		Client *tmp = *start;
		while (tmp->next != NULL)
		{
			tmp = tmp->next;
		}
		tmp->next = new;
	}
	return new;
}

void PrintQueue(Client *start)
{
	Client *tmp = NULL;
	if(start == NULL)
	{
		return;
	}
	else
	{
		tmp = start;
		while(tmp!=NULL)
		{
			printf("%d ", tmp->id);
			tmp = tmp->next;
		}
		
	}
	printf("\n");
}

void PrintDebug()
{
	printf("Waiting: ");
	PrintQueue(barberQueue);
	printf("Resigned: ");
	PrintQueue(resignedClients);
	printf("\n\n");
	
}


void Barber()
{
	while (1)
	{
		pthread_mutex_lock(&queuechairs); //fryzjer wzywa klienta - zmiana stanu krzesel z poczekalni
		while (waitingclients == 0)
        	{
            		currentclientid  = 0;
            		pthread_cond_wait(&clientsAvailable, &queuechairs );
        	}
		waitingclients--;
		Client *client = barberQueue;
		barberQueue = barberQueue->next;
		currentclientid=client->id;
		printf("Klient %d siada na krzeslo\n",currentclientid);
		if(currentclientid==0)
		{
		
			printf("Res: %d WRomm% d/%d [in: -]\n",resignedclients,waitingclients,chairs);
		}
		else
		{
			printf("Res: %d WRomm% d/%d [in: %d]\n",resignedclients,waitingclients,chairs,currentclientid);
		}
		
		
		pthread_mutex_unlock(&queuechairs); //koniec zmiany stanu krzesel
		pthread_cond_signal(&client->turn); // sygnalizuje gotowsc do strzyzenia
		
		
		pthread_mutex_lock(&gettingCut); // zaczyna sie strzyzenie
		int i=rand()%5;
		sleep(i);
		pthread_cond_signal(&barberCutting);
		pthread_mutex_unlock(&gettingCut); // sygnalizuje zakonczenie strzyzenia
	}
}

void Customer()
{
	pthread_mutex_lock(&queuechairs); //Klient wchodzi do salonu i chce zajac krzeslo - zmiana stanu wolnych krzesel
	totalclients++;
	printf("Nowy klient %d wchodzi do salonu\n", totalclients);
	int clientid = totalclients;

	if(waitingclients<chairs) //jezeli liczba czekajacych klientow jest mniejsza od liczby krzesel to klient ktory wszedl zajmuje wolne miejsce w poczekalni
	{
		waitingclients++;
		syslog(LOG_INFO, "Nowy klient o id %d czeka w poczekalni.\n", clientid);
		Client *newClient = addClientToQueue(&barberQueue, clientid);

		
		if(currentclientid==0)
		{
		
			printf("Res: %d WRomm% d/%d [in: -]\n",resignedclients,waitingclients,chairs);
		}
		else
		{
			printf("Res: %d WRomm% d/%d [in: %d]\n",resignedclients,waitingclients,chairs,currentclientid);
		}

		pthread_cond_signal(&clientsAvailable); // inforamuje ze czeka klient

		pthread_mutex_unlock(&queuechairs); //odblowanie dosteppu

		pthread_mutex_lock(&accessBarber); // czeka na nasz kolej
 
		while (currentclientid != clientid)
		{

			pthread_cond_wait(&newClient->turn, &accessBarber);
		}

		pthread_mutex_unlock(&accessBarber); // czekamy na strzyzenie

		pthread_mutex_lock(&gettingCut);
		
			pthread_cond_wait(&barberCutting, &gettingCut); //czekamy na ostrzyzenie
		
		pthread_mutex_unlock(&gettingCut);
	}
	else //jezeli liczba czekajacych klientow jest wieksza od liczby krzesel to klient rezygnuje
	{
		addClientToQueue(&resignedClients, clientid); //dadajemy zrezygnowanego klineta do listy
		resignedclients++;
		printf("Klient %d rezygnuje.\n",clientid);
		if(currentclientid==0)
		{
		
			printf("Res: %d WRomm% d/%d [in: -]\n",resignedclients,waitingclients,chairs);
		}
		else
		{
			printf("Res: %d WRomm% d/%d [in: %d]\n",resignedclients,waitingclients,chairs,currentclientid);
		}
		pthread_mutex_unlock(&queuechairs); //koniec zmiany stanu krzesel
	}
}

int main(int argc, char *argv[])
{
	pthread_t thread1, thread2;
	srand(time(NULL));
	openlog("PROJEKT", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
	int c = 0;

	
	if(argc==1)
	{
		syslog(LOG_INFO, "Blad: Nie wlasciwa liczba argumentow");
		
	}
	else if(argc>=2)
	{
		chairs = atoi(argv[1]);
		syslog(LOG_INFO, "Liczba krzesel: %d\n", chairs);
	}
	else
	{
		syslog(LOG_INFO, "Blad: Nie wlasciwa liczba argumentow.\n");
		printf("blad");
		exit(EXIT_FAILURE);
	}
	//Utworzenie 
	//watku fryzjera
	int iret;
	iret = pthread_create(&thread1, NULL, (void *)Barber, NULL);
	if (iret)
	{
		syslog(LOG_INFO, "Blad: blad przy tworzeniu watku fryzjera\n");
		exit(EXIT_FAILURE);
	}
	//Utworzenie watkow klientow - od 0 do 4 sekund tworzy sie nowy watek
	while (1)
	{
		sleep(rand() % 5);
		iret = pthread_create(&thread2, NULL, (void *)Customer, NULL);
		if (iret)
		{
			syslog(LOG_INFO, "Blad: blad przy tworzeniu watku klienta\n");
			exit(EXIT_FAILURE);
		}
	}


	closelog();
	return EXIT_SUCCESS;
}
