#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int new_voter=-1;
int waiting_voter=-2;
int assigned_voter=+1;
int completed_voter=+2;

typedef struct Booth Booth;
typedef struct EVM EVM;
typedef struct Voter Voter;

struct EVM
{
	int idx;
	pthread_t evm_thread_id;
	Booth * booth;
	int number_of_slots;
	int flag;
};

struct Voter
{
	int idx;
	pthread_t voter_thread_id;
	Booth * booth;
	EVM * evm;
	int status;
};

struct Booth
{
	int idx;
	pthread_t booth_thread_id;
	int evm;
	int evm_slots;
	int voter;
	int done_voters;
	EVM ** evms;
	Voter ** voters;
	pthread_mutex_t mutex;
	pthread_cond_t cv_1;
	pthread_cond_t cv_2;
};

Voter* voter_init(Voter* voter,Booth* booth,int idx);

EVM* evm_init(EVM* evm,Booth* booth,int idx);

Booth* booth_init( Booth * booth, int evm, int evm_slots, int voter,int idx);

void * voter_thread(void * args);

void * evm_thread(void * args);

void * booth_thread(void* args);

int main()
{
	int number_of_booths,total_evms,total_voters,max_evm_slots;
	printf("Enter number of cabs:");
	scanf("%d", &number_of_booths);

	int * evm = (int*)malloc(sizeof(int)*number_of_booths);
	int * voter = (int*)malloc(sizeof(int)*number_of_booths);
	int * evm_slots = (int*)malloc(sizeof(int)*number_of_booths);

	for(int i=0; i<number_of_booths; i++)
	{
		printf("Enter number of voters for booth %d:",i);
		scanf("%d", &total_voters);
		printf("Enter number of evms for booth %d:",i);
		scanf("%d", &total_evms);
		printf("Enter max slots evm for booth %d(less than or equal to 10):",i);
		scanf("%d", &max_evm_slots);
		evm[i] = total_evms;
		evm_slots[i] = max_evm_slots;
		voter[i] = total_voters;
	}
	Booth ** booths = (Booth**)malloc(sizeof(Booth*)*number_of_booths);
	printf("Lets start the election	n.\n");
	for(int i=0; i<number_of_booths; i++)
	{
		booths[i] = booth_init( booths[i],evm[i],evm_slots[i],voter[i],i);
	}
	for(int i=0; i<number_of_booths; i++)
	{
		pthread_create(&(booths[i]->booth_thread_id),NULL, booth_thread, booths[i]);
	}
	for(int i=0; i<number_of_booths; i++)
	{
		pthread_join(booths[i]->booth_thread_id, 0);
	}
	printf("Atlast it ends.\n");
	return 0;
}

Voter* voter_init(Voter* voter,Booth* booth,int idx)
{
	voter = (Voter*)malloc(sizeof(Voter));

	voter->evm = NULL;
	voter->booth = booth;
	voter->status = new_voter;
	voter->idx = idx;

	return voter;
}

void * voter_thread(void * args)
{
	Voter * voter = (Voter*)args;

	pthread_cond_t * cv_1_ptr = &(voter->booth->cv_1);
	pthread_cond_t * cv_2_ptr = &(voter->booth->cv_2);
	pthread_mutex_t * mutex_ptr = &(voter->booth->mutex);

	printf("Voter %d waiting for EVM\n",voter->idx);

	pthread_mutex_lock(mutex_ptr);

	voter->status = waiting_voter;

	while(1)
	{
		if(voter->status!=waiting_voter)
		{
			break;
		}
		else
		{
			pthread_cond_wait(cv_1_ptr, mutex_ptr);
		}
	}

	pthread_mutex_unlock(mutex_ptr);

	EVM * evm = voter->evm;

	printf("Voter %d is in the slot waiting for EVM to start\n",voter->idx);

	pthread_mutex_lock(mutex_ptr);

	while(1)
	{
		if(evm->flag == 0)
		{
			pthread_cond_wait(cv_1_ptr, mutex_ptr);
		}
		else
		{
			break;
		}
	}

	(evm->number_of_slots)-=1;

	printf("%d voter has casted its vote in %d booth of %d evm.\n",voter->idx,evm->booth->idx,evm->idx);

	pthread_cond_broadcast(cv_2_ptr);
	pthread_mutex_unlock(mutex_ptr);
}

EVM* evm_init(EVM * evm,Booth * booth,int idx)
{
	evm = (EVM*)malloc(sizeof(EVM));

	evm->number_of_slots = 0;
	evm->booth = booth;
	evm->flag = 0;
	evm->idx = idx;

	return evm;
}

void * evm_thread(void * args)
{
	EVM * evm = (EVM*)args;
	Booth * booth = evm->booth;

	pthread_cond_t * cv_1_ptr = &(booth->cv_1);
	pthread_mutex_t * mutex_ptr = &(booth->mutex);
	pthread_cond_t * cv_2_ptr = &(booth->cv_2);


	int i, j, max_slts;

	while(1)
	{
		if(booth->done_voters == booth->voter)
		{
			pthread_mutex_unlock(mutex_ptr);
			break;
		}
		else
		{
			printf("EVM %d of Booth %d getting free slots to allot to user\n",evm->idx,booth->idx);
			j = 0;
			pthread_mutex_lock(mutex_ptr);
			max_slts = rand()%(booth->evm_slots) + 1;
			evm->flag = 0;
			evm->number_of_slots = max_slts;
			pthread_mutex_unlock(mutex_ptr);
			printf("%d Booth's %d EVM has %d slots free.\n", booth->idx, evm->idx, max_slts);

			while(1)
			{
				if(j>=max_slts)
				{
					break;
				}
				else
				{
					pthread_mutex_lock(mutex_ptr);
					i = rand()%booth->voter;
					if(booth->voters[i]->status == waiting_voter)
					{
						booth->voters[i]->evm = evm;
						(booth->done_voters)+=1;
						booth->voters[i]->status = assigned_voter;
						printf("%d Voter is alloted %d Booth's %d EVM.\n",i,booth->idx,evm->idx);
						j++;
					}
					if(booth->done_voters == booth->voter)
					{
						pthread_mutex_unlock(mutex_ptr);
						break;
					}
					pthread_mutex_unlock(mutex_ptr);
				}
			}
			if(j==0)
			{
				break;
			}
			pthread_mutex_lock(mutex_ptr);
			printf("%d Booth's %d EVM has started voting phase.\n", booth->idx, evm->idx);
			evm->number_of_slots = j;
			evm->flag = 1;
			pthread_cond_broadcast(cv_1_ptr);
			while(1)
			{
				if(evm->number_of_slots==0)
				{
					break;
				}
				else
				{
					pthread_cond_wait(cv_2_ptr, mutex_ptr);
				}
			}
			pthread_mutex_unlock(mutex_ptr);
			printf("%d Booth's %d EVM has finished voting phase.\n", booth->idx, evm->idx);
		}
	}
	printf("%d Booth's %d EVM signing off.\n", booth->idx, evm->idx);
	return NULL;
}

Booth* booth_init( Booth * booth,int evm, int evm_slots, int voter,int idx)
{
	booth = (Booth*)malloc(sizeof(Booth));
	booth->evms = (EVM**)malloc(sizeof(EVM*)*evm);
	booth->voters = (Voter**)malloc(sizeof(Voter*)*voter);

	booth->done_voters = 0;
	booth->evm_slots = evm_slots;
	booth->voter = voter;
	booth->evm = evm;
	booth->idx = idx;

	pthread_mutex_init(&(booth->mutex), NULL);
	pthread_cond_init(&(booth->cv_1), NULL);
	pthread_cond_init(&(booth->cv_2), NULL);

	return booth;
}

void * booth_thread(void* args)
{
	Booth * booth = (Booth*)args;

	printf("Creating EVM and VOTER threads and joining them\n");

	for(int i=0; i<booth->evm; i++)
	{
		booth->evms[i] = evm_init(booth->evms[i],booth,i);
		pthread_create(&(booth->evms[i]->evm_thread_id),NULL,evm_thread,booth->evms[i]);
		pthread_join(booth->evms[i]->evm_thread_id, 0);
	}
	for(int i=0; i<booth->voter; i++)
	{
		booth->voters[i] = voter_init(booth->voters[i],booth,i);
		pthread_create(&(booth->voters[i]->voter_thread_id),NULL, voter_thread, booth->voters[i]);
		pthread_join(booth->voters[i]->voter_thread_id, 0);
	}

	printf("BOOTH %d is done.\n", booth->idx);

	return NULL;
}