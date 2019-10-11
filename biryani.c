#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>

int no_chefs, no_students, no_tables;

int main()
{
    printf("Enter number of chefs, tables, and students:\n");
    scanf("%d %d %d", &no_chefs, no_tables, no_students);    
}