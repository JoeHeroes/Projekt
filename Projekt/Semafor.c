#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <unistd.h>
#include <syslog.h>
int chairs;
volatile int currentclientid=0;
sem_t customerReady;
pthread_mutex_t queuechairs=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gettingCut=PTHREAD_MUTEX_INITIALIZER;
int waitingclients =0; //liczba czekajacych klientow
int totalclients=0; //liczba wszystkich klientow ktorzy byli w salonie
int resignedclients=0; //liczba klientow ktorzy zrezygnowali
typedef struct Client_Struct
{
	int id;
	sem_t turn;
	sem_t wasCut;
	struct Client_Struct *next;
}Client;

Client *barberQueue=NULL;
Client *resignedClients=NULL;

Client* AddClientToQueue(Client **start,int id)
{
	Client *new=malloc(sizeof(Client));
	new->id=id;
	new->next=NULL;
	sem_init(&new->turn,0,0);
	sem_init(&new->wasCut,0,0);
	if(*start==NULL)
	{
		(*start)=new;
	}
	else
	{
		Client *tmp=*start;
		while(tmp->next!=NULL)
		{
			tmp=tmp->next;
		}
		tmp->next=new;
	}
	return new;
}
void Barber()
{
	while(1)
	{
		sem_wait(&customerReady); // fryzjer czeka na klienta - jezeli nie ma klienta to spi
		pthread_mutex_lock(&queuechairs); //fryzjer wzywa klienta - zmiana stanu krzesel z poczekalni
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
		sem_post(&client->turn); //nastepuje strzyzenie
		pthread_mutex_lock(&gettingCut);
		int i=rand()%5;
		sleep(i);
		sem_post(&client->wasCut);
		pthread_mutex_unlock(&gettingCut);
		currentclientid=0;
		
	}
}
void Customer()
{
	pthread_mutex_lock(&queuechairs); //Klient wchodzi do salonu i chce zajac krzeslo - zmiana stanu wolnych krzesel
	totalclients++;
	printf("Klient %d wchodzi do salonu.\n",totalclients);
	int clientid=totalclients;
	if(waitingclients<chairs) //jezeli liczba czekajacych klientow jest mniejsza od liczby krzesel to klient ktory wszedl zajmuje wolne miejsce w poczekalni
	{
		waitingclients++;
		syslog(LOG_INFO,"Nowy klient o id %d czeka w poczekalni.\n",clientid);
		Client *newClient=AddClientToQueue(&barberQueue,clientid);
		
		//Wypisanie informacji
		if(currentclientid==0)
		{
			printf("Res: %d WRomm% d/%d [in: -]\n",resignedclients,waitingclients,chairs);
		}
		else
		{
			printf("Res: %d WRomm% d/%d [in: %d]\n",resignedclients,waitingclients,chairs,currentclientid);
		}
		
		sem_post(&customerReady); //klient oczekuje na strzyzenie
		pthread_mutex_unlock(&queuechairs); //koniec zmiany stanu krzesel
		sem_wait(&newClient->turn);//klient czeka na swoja kolej
		sem_wait(&newClient->wasCut);//czekanie na skonczenie strzyzenia
		
	}
	else //jezeli liczba czekajacych klientow jest wieksza od liczby krzesel to klient rezygnuje
	{
		resignedclients++;
		Client *newClient=AddClientToQueue(&resignedClients,clientid);
		printf("Klient %d rezygnuje.\n",clientid);
		if(currentclientid==0)
		{
			printf("Res: %d WRomm% d/%d\n [in: -]",resignedclients,waitingclients,chairs);
		}
		else
		{
			printf("Res: %d WRomm% d/%d [in: %d]\n",resignedclients,waitingclients,chairs,currentclientid);
		}	
		pthread_mutex_unlock(&queuechairs); //koniec zmiany stanu krzesel
	}
}
int main (int argc, char* argv[])
{
    pthread_t thread1, thread2;
    srand(time(NULL));
    openlog("PROJEKT",LOG_CONS|LOG_PID|LOG_NDELAY,LOG_LOCAL1);
    int c = 0;
    
    if(argc!=2)
    {
	syslog(LOG_INFO, "Blad: Nie wlasciwa liczba argumentow.\n");
	printf("Error");
 	exit(EXIT_FAILURE);
    }
    
    chairs = atoi(argv[1]);
    syslog(LOG_INFO, "Liczba krzesel: %d\n", chairs);
   
    
    
    //Inicjalizacja semaforu
    if(sem_init(&customerReady,0,0)!=0)
    {
    syslog(LOG_INFO,"Blad: Blad z inicjalizacja semaforu.\n");
    };
    //Utworzenie watku fryzjera
    int iret;
    iret = pthread_create( &thread1, NULL, (void*)Barber,NULL);
    if(iret) {
    syslog(LOG_INFO,"Blad: blad przy tworzeniu watku fryzjera\n");
    exit(EXIT_FAILURE);
    }
    //Utworzenie watkow klientow - od 0 do 4 sekund tworzy sie nowy watek
    while(1)
    {
    	sleep(rand()%3);
    	iret = pthread_create( &thread2, NULL, (void*)Customer,NULL);
    	if(iret) 
    	{
    		syslog(LOG_INFO,"Blad: blad przy tworzeniu watku klienta\n");
    		exit(EXIT_FAILURE);
    	}
    }
    closelog();
    return EXIT_SUCCESS;
}
