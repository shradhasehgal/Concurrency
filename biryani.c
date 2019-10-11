#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>

int no_chefs, no_students, no_tables;

typedef struct Chef Chef;
typedef struct Table Table;
typedef struct Student Student;

struct Chef
{
	int idx;
	int status;
    int vessels_left;
    int capacity;
	pthread_mutex_t chef_mutex;
    pthread_t chef_thread_id;
    pthread_cond_t cv_table; 
};

struct Table
{
    int idx;
    int capacity;
    int slots;
    int occupancy;
    pthread_mutex_t table_mutex;
    pthread_t table_thread_id;
    pthread_cond_t cv_student;
};

struct Student
{
    int idx;
    pthread_mutex_t student_mutex;
    pthread_t student_thread_id;
};

Chef chefs[1000]; Table tables[1000]; Student students[1000]; 

void * chef_thread(void* args);
void * table_thread(void* args);
void * student_thread(void* args);
void biryani_ready(Chef *chef);
void ready_to_serve_table(Table *table);

int main()
{
    srand(time(0));
    printf("Enter number of chefs, tables, and students:\n");
    scanf("%d %d %d", &no_chefs, &no_tables, &no_students);

    for(int i=0; i < no_chefs; i++)
    {
        chefs[i].idx = i+1;
        pthread_mutex_init(&(chefs[i].chef_mutex), NULL);
    }   

    for(int i=0; i < no_tables; i++)
    {
        tables[i].idx = i+1;
        pthread_mutex_init(&(chefs[i].chef_mutex), NULL);
    }  

    for(int i=0; i < no_students; i++)
    {
        students[i].idx = i+1;
        pthread_mutex_init(&(chefs[i].chef_mutex), NULL);
    }

    printf("Beginning Simulation\n\n");

    for(int i=0; i < no_chefs; i++)
		pthread_create(&(chefs[i].chef_thread_id), NULL, chef_thread , &chefs[i]);

	for(int i=0; i < no_tables; i++)
		pthread_create(&(tables[i].table_thread_id), NULL, table_thread , &tables[i]);

    for(int i=0; i < no_students; i++)
		pthread_create(&(students[i].student_thread_id), NULL, student_thread , &students[i]);

	for(int i=0; i < no_students; i++)
		pthread_join(students[i].student_thread_id, 0);  

    for(int i=0; i< no_chefs; i++)
		pthread_mutex_destroy(&(chefs[i].chef_mutex));
    
    for(int i=0; i < no_tables; i++)
		pthread_mutex_destroy(&(tables[i].table_mutex));

    printf("\nSimulation over!\n");
	
	return 0; 
}

void * chef_thread(void* args)
{
	Chef * chef = (Chef*)args;
    
    while(1)
    {
        int w = rand()%4 + 2;
        sleep(w);
        pthread_mutex_lock(&(chef -> chef_mutex));
        chef->vessels_left = rand()%10 + 1;
        chef->capacity = rand()%6 + 20;
        printf("CHEF %d HAS PREPARED %d VESSELS WITH CAPACITY %d\n",chef->idx, chef->vessels_left, chef->capacity);        
        biryani_ready(chef);
    }

    return NULL;
}

void biryani_ready(Chef *chef)
{
    while(1)
    {
        if(chef -> vessels_left == 0)
            break;
        else pthread_cond_wait(&(chef->cv_table), &(chef->chef_mutex));
    }

    pthread_mutex_unlock(&(chef -> chef_mutex));
    printf("CHEF %d RESUMING COOKING AS ALL VESSELS EMPTIED\n", chef->idx);
}

void * table_thread(void* args)
{
    Table * table = (Table*)args;
    
    while(1)
    {
        for(int i=0; i < no_chefs; i++)
        {
            pthread_mutex_lock(&(chefs[i].chef_mutex));
            if(chefs[i].vessels_left > 0)
            {
                table -> capacity = chefs[i].capacity;
                chefs[i].vessels_left--;
                printf("TABLE %d HAS RECEIVED VESSEL FROM CHEF %d\n", table->idx, i+1);
                pthread_cond_signal(&(chefs[i].cv_table));
                pthread_mutex_unlock(&(chefs[i].chef_mutex));
                break;
            }

            pthread_cond_signal(&(chefs[i].cv_table));
            pthread_mutex_unlock(&(chefs[i].chef_mutex));
        }

        while(1)
        {
            pthread_mutex_lock(&(table->table_mutex));
            
            if(table->capacity == 0)
            {
                pthread_mutex_unlock(&(table->table_mutex));
                break;
            }
            
            table -> slots = rand() % 10 + 1;
            table -> occupancy = 0;

            if(table -> slots > table -> capacity)
                table-> slots = table -> capacity;

            table -> capacity -= table->slots;
            printf("TABLE %d READY WITH SLOTS %d\n", table->idx, table->slots);
            ready_to_serve_table(table);
        }
        
    }

    return NULL;
}

void ready_to_serve_table(Table *table)
{
    while(1)
    {
        if(table -> slots == table -> occupancy)
            break;
        else pthread_cond_wait(&(table-> cv_student), &(table-> table_mutex));
    }

    pthread_mutex_unlock(&(table -> table_mutex));
}

void * student_thread(void* args)
{
    Student * student = (Student*)args;
    int arrival_time = rand()%10;
    sleep(arrival_time);
    printf("STUDENT %d HAS ARRIVED\n", student->idx);

    int got_food =0;
    while(!got_food)
    {
        for(int i=0; i < no_tables; i++)
        {
            pthread_mutex_lock(&(tables[i].table_mutex));
            if(tables[i].slots - tables[i].occupancy > 0)
            {
                tables[i].occupancy++;
                printf("STUDENT %d EATING AT TABLE %d\n",student->idx, i+1);
                got_food = 1;
                pthread_cond_signal(&(tables[i].cv_student));
                pthread_mutex_unlock(&(tables[i].table_mutex));
                break;
            }

            pthread_cond_signal(&(tables[i].cv_student));
            pthread_mutex_unlock(&(tables[i].table_mutex));
        }
    }

    return NULL;
}