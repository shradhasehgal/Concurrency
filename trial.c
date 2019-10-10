#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>

typedef struct Cab Cab;
typedef struct Rider Rider;
sem_t premier_cabs;
sem_t pool_cabs;
sem_t servers;

pthread_mutex_t mutex;

struct Cab
{
	int type;
	int status;
	int r1, r2;
};

Cab Cabs[1000];
int no_cabs,no_riders,no_servers;

struct Rider
{
	int idx;
	pthread_t rider_thread_id;
	int arrivalTime;
	int cabType;
	int maxWaitTime;
	int RideTime;
};

void * rider_thread(void* args);

int main()
{
	printf("Enter number of cabs, riders, and payment servers:\n");
	scanf("%d %d %d", &no_cabs, &no_riders, &no_servers);

	Rider *riders = malloc((no_riders+2) * sizeof(Rider));
	srand(time(0));
	sem_init(&premier_cabs, 0, no_cabs);
	sem_init(&pool_cabs, 0, no_cabs); 
	sem_init(&servers, 0, no_servers); 
	pthread_mutex_init(&mutex, NULL);

	for(int i=0; i < no_riders; i++)
	{
		riders[i].idx = i; 	
		riders[i].arrivalTime = rand()%3; 
		riders[i].cabType = rand()%2+1;
		riders[i].maxWaitTime = rand()%10;
		riders[i].RideTime = rand()%15;
		printf("%d. %d\n", riders[i].idx,riders[i].arrivalTime);
		pthread_create(&(riders[i].rider_thread_id), NULL, rider_thread , &riders[i]);
	}

	for(int i=0; i < no_riders; i++)
	{
		pthread_join(riders[i].rider_thread_id, 0);
	}
	pthread_mutex_destroy(&mutex);
	free(riders);
	return 0;
}


void * rider_thread(void* args)
{
	Rider * rider = (Rider*)args;
	sleep(rider ->arrivalTime);
	
	if(rider ->cabType == 1)
	{
		sem_wait(&premier_cabs); sem_wait(&pool_cabs);
		//mutex_lock(semaphore->mutex);
		pthread_mutex_lock(&mutex);

		for(int i=0; i < no_cabs; i++)
		{
			if(Cabs[i].type==0)
			{
				Cabs[i].type = 1;
				break;
			}

		}
		printf("Rider %d has begun ride in a primer cab!", rider->idx);
		
		pthread_mutex_unlock(&mutex);
		
		sleep(rider->RideTime);	
		
		printf("Rider %d has finished ride in premier cab!", rider->idx);
		
		sem_post(&premier_cabs); sem_post(&pool_cabs);
		
		sem_wait(&servers); //sem_wait(&pool_cabs);
		printf("Rider %d making payment!", rider->idx);
		sleep(2);
		sem_post(&servers);
		//mutex_unlock(semaphore->mutex);
	}

	if(rider ->cabType == 2)
	{
		sem_wait(&pool_cabs);

		int f = 0;
		for(int i=0; i < no_cabs; i++)
		{
			if(Cabs[i].type == 2 && Cabs[i].status == 1)
			{
				f = 1;
				pthread_mutex_lock(&mutex);
				Cabs[i].status = 2;
				//cabs[i].r2 = rider ->idx;
				printf("Rider %d has sat in pool cab!\n", rider ->idx);
				pthread_mutex_unlock(&mutex);
				
				sleep(rider->RideTime);
				pthread_mutex_lock(&mutex);
				printf("Rider %d has finished ride!\n", rider ->idx);
				
				if(Cabs[i].status == 1)	
				{
					Cabs[i].type = 0;
					sem_post(&premier_cabs);
				}
				
				Cabs[i].status--;

				sem_post(&pool_cabs);
				pthread_mutex_unlock(&mutex);
				
				sem_wait(&servers); //sem_wait(&pool_cabs);
				printf("Rider %d making payment!", rider->idx);
				sleep(2);
				sem_post(&servers);
			}

			break;

		}

		if(f==0)
		{
			sem_wait(&premier_cabs); sem_post(&pool_cabs); 		
			
			for(int i=0; i < no_cabs; i++)
			{
				if(Cabs[i].type == 0)
				{
					pthread_mutex_lock(&mutex);
					Cabs[i].type = 2;
					//cabs[i].r1 = rider->idx;
					Cabs[i].status = 1;
					printf("Rider %d has sat in pool cab!\n", rider ->idx);
					pthread_mutex_unlock(&mutex);
					sleep(rider->RideTime);

					printf("Rider %d has finished ride!\n", rider ->idx);
					
					if(Cabs[i].status == 1)
					{
						sem_post(&premier_cabs);
						Cabs[i].status = 0;
						Cabs[i].type = 0;
					}

					sem_wait(&servers); //sem_wait(&pool_cabs);
					printf("Rider %d making payment!", rider->idx);
					sleep(2);
					sem_post(&servers);

				}

				break;
			}
		}

	}
	

	//sleep(rider->RideTime);
	 
	//pthread_mutex_unlock(&mutex);
	
	return NULL;
}