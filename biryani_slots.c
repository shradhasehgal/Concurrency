#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>


#define KNRM  "\x1B[1;0m"
#define KRED  "\x1B[1;31m"
#define KGRN  "\x1B[1;32m"
#define KYEL  "\x1B[1;33m"
#define KBLU  "\x1B[1;34m"
#define KMAG  "\x1B[1;35m"
#define KCYN  "\x1B[1;36m"
#define KWHT  "\x1B[1;37m"
#define KGR "\x1B[1;32m"

int no_chefs, no_students, no_tables;

typedef struct Chef Chef;
typedef struct Table Table;
typedef struct Student Student;
pthread_mutex_t waiting_mutex;

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
int waiting_students;

void * chef_thread(void* args);
void * table_thread(void* args);
void * student_thread(void* args);
void biryani_ready(Chef *chef);
void ready_to_serve_table(Table *table);
void wait_for_slot(int idx);
void student_in_slot(int i, int idx);

int main()
{
    srand(time(0));
    printf("Enter number of chefs, tables, and students:\n");
    scanf("%d %d %d", &no_chefs, &no_tables, &no_students);

    waiting_students = 0;
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

    printf("%sBeginning Simulation\n\n",KGR);

    for(int i=0; i < no_chefs; i++)
		pthread_create(&(chefs[i].chef_thread_id), NULL, chef_thread , &chefs[i]);

	for(int i=0; i < no_tables; i++)
		pthread_create(&(tables[i].table_thread_id), NULL, table_thread , &tables[i]);

    for(int i=0; i < no_students; i++)
		pthread_create(&(students[i].student_thread_id), NULL, student_thread , &students[i]);

	for(int i=0; i < no_students; i++)
		pthread_join(students[i].student_thread_id, 0);  

    // for(int i=0; i < no_tables; i++)
    // {
    //     if(tables[i].occupancy > 0)
    //     {
    //         printf("%sTABLE %d SERVING\n",KCYN, i+1);
    //     }
    // }
    printf("\n%sSimulation over!\n", KGR);

    for(int i=0; i< no_chefs; i++)
		pthread_mutex_destroy(&(chefs[i].chef_mutex));
    
    for(int i=0; i < no_tables; i++)
		pthread_mutex_destroy(&(tables[i].table_mutex));

	
	return 0; 
}

void * chef_thread(void* args)
{
	Chef * chef = (Chef*)args;
    
    while(1)
    {
        int r = rand()%10 + 1;
        printf("%sCHEF %d PREPARING %d VESSELS\n",KRED, chef->idx, r);
        int w = rand()%4 + 2;
        sleep(w);
        pthread_mutex_lock(&(chef -> chef_mutex));
        chef->vessels_left = r;
        chef->capacity = rand()%6 + 20;
        printf("%sCHEF %d HAS PREPARED %d VESSELS WITH CAPACITY %d\n",KRED, chef->idx, chef->vessels_left, chef->capacity);        
        biryani_ready(chef);
    }

    return NULL;
}

void biryani_ready(Chef *chef)
{
    while(1)
    {
        if(chef -> vessels_left == 0)
        {
            break;
        }
        else pthread_cond_wait(&(chef->cv_table), &(chef->chef_mutex));
    }

    printf("%sALL VESSELS PREPARED BY CHEF %d ARE EMPTY, RESUMING COOKING NOW\n",KYEL,chef->idx);
    pthread_mutex_unlock(&(chef -> chef_mutex));
    //printf("%sCHEF %d RESUMING COOKING AS ALL VESSELS EMPTIED\n",KNRM ,chef->idx);
}

void * table_thread(void* args)
{
    Table * table = (Table*)args;
    
    while(1)
    {
        int fluff = 0;
        for(int i=0; i < no_chefs; i++)
        {
            pthread_mutex_lock(&(chefs[i].chef_mutex));
            if(chefs[i].vessels_left > 0)
            {
                fluff = 1;
                table -> capacity = chefs[i].capacity;
                chefs[i].vessels_left--;
                printf("%sTABLE %d HAS RECEIVED VESSEL FROM CHEF %d\n", KCYN,table->idx, i+1);
                pthread_cond_signal(&(chefs[i].cv_table));
                pthread_mutex_unlock(&(chefs[i].chef_mutex));
                break;
            }

            pthread_cond_signal(&(chefs[i].cv_table));
            pthread_mutex_unlock(&(chefs[i].chef_mutex));
        }

        while(fluff)
        {
            pthread_mutex_lock(&(table->table_mutex));
            
            if(table->capacity == 0)
            {
                printf("%sSERVING CONTAINER AT TABLE %d EMPTY, WAITING FOR REFILL\n",KRED, table->idx);
                pthread_mutex_unlock(&(table->table_mutex));
                break;
            }
            
            table -> slots = rand() % 10 + 1;
            table -> occupancy = 0;

            if(table -> slots > table -> capacity)
                table-> slots = table -> capacity;

            table -> capacity -= table->slots;
            printf("%sTABLE %d READY TO SERVE WITH %d SLOTS\n",KMAG,table->idx, table->slots);
            ready_to_serve_table(table);
        }
        
    }

    return NULL;
}

void ready_to_serve_table(Table *table)
{
    while(1)
    {
        pthread_mutex_lock(&(waiting_mutex));
        if(waiting_students == 0)
        {
            printf("%sNO WAITING STUDENTS CURRENTLY, TABLE %d SLOTS EMPTIED\n",KYEL,table->idx);
            pthread_mutex_unlock(&(waiting_mutex));
            
            break;
        }

        pthread_mutex_unlock(&(waiting_mutex));

        if(table -> slots == table -> occupancy)
        {
            printf("%sALL SLOTS FILLED FOR TABLE %d, RECHECKING CAPACITY\n",KCYN,table->idx);
            break;
        }

        else pthread_cond_wait(&(table-> cv_student), &(table-> table_mutex));
    }

    pthread_mutex_unlock(&(table -> table_mutex));
}

void * student_thread(void* args)
{
    Student * student = (Student*)args;
    int arrival_time = rand()%10;
    sleep(arrival_time);
    pthread_mutex_lock(&(waiting_mutex));
    waiting_students++;
    pthread_mutex_unlock(&(waiting_mutex));
    printf("%sSTUDENT %d HAS ARRIVED AND IS WAITING FOR SLOT\n",KYEL,student->idx);
    wait_for_slot(student->idx);
    return NULL;
}

void wait_for_slot(int idx)
{
    int got_table =0;
    while(!got_table)
    {
        for(int i=0; i < no_tables; i++)
        {
            pthread_mutex_lock(&(tables[i].table_mutex));
            if(tables[i].slots - tables[i].occupancy > 0)
            {
                tables[i].occupancy++;
                got_table = 1;
                student_in_slot(i, idx);
                break;
            }

            pthread_cond_signal(&(tables[i].cv_student));
            pthread_mutex_unlock(&(tables[i].table_mutex));
        }
    }

    return;
}

void student_in_slot(int i, int idx)
{
    pthread_mutex_lock(&(waiting_mutex));
    waiting_students--;
    pthread_mutex_unlock(&(waiting_mutex));
    printf("%sSTUDENT %d WAITING FOR SLOT AT TABLE %d\n",KRED,idx, i+1);
    printf("%sSTUDENT %d EATING AT TABLE %d\n",KGRN,idx, i+1);
    pthread_cond_signal(&(tables[i].cv_student));
    pthread_mutex_unlock(&(tables[i].table_mutex));
    return;
}