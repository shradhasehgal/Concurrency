#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>

typedef struct Cab Cab;
typedef struct Rider Rider;
//sem_t cabs;
sem_t servers;

pthread_mutex_t mutex;


struct Cab
{
	int type;
	int status;
	pthread_mutex_t cab_mutex;
};

Cab Cabs[1000];
int no_cabs,no_riders,no_servers;
pthread_mutex_t pay_servers[1000];

struct Rider
{
	int idx;
	pthread_t rider_thread_id;
	int arrivalTime;
	int cabType;
	int maxWaitTime;
	int RideTime;
	int cab_no;
};

void * rider_thread(void* args);

int main()
{
	printf("Enter number of cabs, riders, and payment servers:\n");
	scanf("%d %d %d", &no_cabs, &no_riders, &no_servers);

	Rider *riders = malloc((no_riders+2) * sizeof(Rider));
	srand(time(0));
	// sem_init(&cabs, 0, no_cabs);
	//sem_init(&pool_cabs, 0, no_cabs); 
	sem_init(&servers, 0, no_servers); 


	pthread_mutex_init(&mutex, NULL);

	for(int i=0; i< no_cabs; i++)
	{
		Cabs[i].type = 0;
		Cabs[i].status = 0;
		pthread_mutex_init(&(Cabs[i].cab_mutex), NULL);
	}

	//printf("Enter Arrival time, cab type, max waiting time, Ride time\n");
	for(int i=0; i < no_riders; i++)
	{
		riders[i].idx = i+1; 	
		// riders[i].arrivalTime = rand()%3; 
		// riders[i].cabType = rand()%2+1;
		// riders[i].maxWaitTime = rand()%10;
		// riders[i].RideTime = rand()%15;
		
		scanf("%d %d %d %d",&riders[i].arrivalTime, &riders[i].cabType,&riders[i].maxWaitTime, &riders[i].RideTime);
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

	// pthread_mutex_destroy(&mutex);
	for(int i=0; i< no_cabs; i++)
		pthread_mutex_destroy(&(Cabs[i].cab_mutex));
	
	free(riders);
	return 0;
}

void payment(int idx)
{
	sem_wait(&servers); //sem_wait(&pool_cabs);
	printf("\x1B[32mRIDER %d PAYING\n\x1B[0m", idx);
	sleep(2);
	printf("\x1B[1;32mRIDER %d DONE!\n\x1B[0m", idx);
	sem_post(&servers);
	return;
}

void * rider_thread(void* args)
{
	Rider * rider = (Rider*)args;
	sleep(rider ->arrivalTime);
	printf("\n\n\x1B[33;1mRider %d \x1B[0mhas arrived with details:\n\x1B[33;1mArrival time - %d\nCab type - %d\nMax Wait Time %d\nRide Time %d\n\n\x1B[0m", rider->idx, rider->arrivalTime,rider->cabType, rider->maxWaitTime, rider->RideTime);
	int got_ride = 0, time_exceed = 0;

	clock_t t = clock();
	
	while(!got_ride)
	{
		clock_t time_taken = clock() - t;
		double times = ((double)time_taken)/CLOCKS_PER_SEC;
		if(times > ((double)rider->maxWaitTime))
		{
			// printf("Time %f\n",times );
			// printf("%d\n",rider->maxWaitTime);
			time_exceed = 1; 
			break;
		}

		int flag = 0;
		for(int i=0; i < no_cabs; i++)
		{
			pthread_mutex_lock(&(Cabs[i].cab_mutex));
			
			if(rider->cabType == 1)
			{
				if(Cabs[i].type == 0)
				{
					//printf("1");
					Cabs[i].type = 1;
					got_ride = 1;
					rider->cab_no = i+1;
					pthread_mutex_unlock(&(Cabs[i].cab_mutex));
					break;
				}
			}

			else if(rider->cabType == 2)
			{
				if(Cabs[i].type == 2 && Cabs[i].status == 1)
				{
					//printf("2");
					Cabs[i].status = 2;
					flag = 1;
					got_ride = 1;
					rider->cab_no = i+1;
					pthread_mutex_unlock(&(Cabs[i].cab_mutex));
					break;
				}
			}
			
			pthread_mutex_unlock(&(Cabs[i].cab_mutex));
		}

		if(rider->cabType == 2 && flag == 0)
		{
			for(int i=0; i < no_cabs; i++)
			{
				pthread_mutex_lock(&(Cabs[i].cab_mutex));
				
				if(Cabs[i].type == 0)
				{
					Cabs[i].type = 2;
					Cabs[i].status = 1;
					got_ride = 1;
					rider->cab_no = i+1;
					pthread_mutex_unlock(&(Cabs[i].cab_mutex));
					break;
				}
			
				
				pthread_mutex_unlock(&(Cabs[i].cab_mutex));
			}
		}
	}
	
	if(time_exceed == 1)
	{
		printf("\e[31;1m TIMEOUT FOR RIDER %d!\x1B[0m\n",rider->idx);
		return NULL;
	}
	
	if(rider->cabType == 1)
		printf("\x1B[1;34mRIDER %d HAS BOARDED PREMIER CAB %d!\x1B[0m\n",rider->idx, rider->cab_no);

	else printf("\x1B[1;34mRIDER %d HAS BOARDED POOL CAB %d!\x1B[0m\n",rider->idx, rider->cab_no);
	

	sleep(rider->RideTime);

	int no = rider->cab_no-1;
	pthread_mutex_lock(&(Cabs[no].cab_mutex));
	if(rider->cabType == 2)
	{
		if(Cabs[no].status==1)
		{
			Cabs[no].type = 0;
			Cabs[no].status = 0;
		}

		else Cabs[no].status = 1;
	}

	else Cabs[no].type = 0;
	printf("\x1B[35;3mRIDER %d HAS FINISHED JOURNEY WITH CAB %d!\x1B[0m\n",rider->idx, rider->cab_no);
	pthread_mutex_unlock(&(Cabs[no].cab_mutex));
	
	payment(rider->idx);

	return NULL;
}