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

pthread_mutex_t mutex_capture;
pthread_mutex_t mutex_release;


struct Cab
{
	int type;
	int status;
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
	pthread_mutex_init(&mutex_capture, NULL);
	pthread_mutex_init(&mutex_release, NULL);

	for(int i=0; i < no_riders; i++)
	{
		riders[i].idx = i; 	
		// riders[i].arrivalTime = rand()%3; 
		// riders[i].cabType = rand()%2+1;
		// riders[i].maxWaitTime = rand()%10;
		// riders[i].RideTime = rand()%15;
		printf("Enter Arrival time, cab type, ridetime\n");
		scanf("%d %d %d",&riders[i].arrivalTime, &riders[i].cabType, &riders[i].RideTime);
		// printf("%d. %d\n\n", riders[i].idx,riders[i].arrivalTime);
		//pthread_create(&(riders[i].rider_thread_id), NULL, rider_thread , &riders[i]);
	}

	for(int i=0; i < no_riders; i++)
	{
		pthread_create(&(riders[i].rider_thread_id), NULL, rider_thread , &riders[i]);
	}

	for(int i=0; i < no_riders; i++)
	{
		pthread_join(riders[i].rider_thread_id, 0);
	}

	pthread_mutex_destroy(&mutex_capture);
	pthread_mutex_destroy(&mutex_release);
	free(riders);
	return 0;
}

void payment(int idx)
{
	sem_wait(&servers); //sem_wait(&pool_cabs);
	sleep(2);
	printf("\x1B[1;32mRider %d has done the payment!\n\n\x1B[0m", idx);
	sem_post(&servers);
	return;
}

void * rider_thread(void* args)
{
	Rider * rider = (Rider*)args;
	sleep(rider ->arrivalTime);
	
	if(rider ->cabType == 1)
	{
		pthread_mutex_lock(&mutex_capture);
		sem_wait(&premier_cabs); sem_wait(&pool_cabs);
		//mutex_lock(semaphore->mutex);

		for(int i=0; i < no_cabs; i++)
		{
			if(Cabs[i].type==0)
			{
				Cabs[i].type = 1;
				break;
			}

		}
		printf("Rider %d has begun ride in a primer cab!\n", rider->idx+1);
		
		pthread_mutex_unlock(&mutex_capture);
		
		sleep(rider->RideTime);	
		pthread_mutex_lock(&mutex_capture);
		printf("Rider %d has finished ride in premier cab!\n", rider->idx+1);
		sem_post(&premier_cabs); sem_post(&pool_cabs);
		pthread_mutex_unlock(&mutex_capture);
		payment(rider->idx);
		//mutex_unlock(semaphore->mutex);
	}

	else if(rider ->cabType == 2)
	{
		sem_wait(&pool_cabs);

		int f = 0;

		pthread_mutex_lock(&mutex_capture);
		
		for(int i=0; i < no_cabs; i++)
		{
			if(Cabs[i].type == 2 && Cabs[i].status == 1)
			{
				f = 1;
				
				Cabs[i].status = 2;
				printf("Rider %d has sat in pool cab!\n", rider ->idx+1);
				break;
			}
		}

		if(f==0)
		{
			sem_wait(&premier_cabs); 
			sem_post(&pool_cabs);

			for(int i=0; i < no_cabs; i++)
			{
				if(Cabs[i].type == 0)
				{
					Cabs[i].type = 2;
					//cabs[i].r1 = rider->idx;
					Cabs[i].status = 1;
				}
			}	

		}
		
		pthread_mutex_unlock(&mutex_capture);
				
		sleep(rider->RideTime);
		printf("Rider %d has finished ride!\n", rider ->idx+1);
				
		if(Cabs[i].status == 1)	
		{
			Cabs[i].type = 0;
			sem_post(&premier_cabs);
		}
		
		Cabs[i].status--;

		sem_post(&pool_cabs);
		//pthread_mutex_unlock(&mutex_release);
		
		payment(rider->idx);
			

	}
	
	
	return NULL;
}