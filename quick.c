#include <sys/types.h> 
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <time.h>
#include <wait.h>
#include <limits.h>
#include <pthread.h>
#include <inttypes.h>
#include <fcntl.h>
#include <math.h>


#define ll long long int
int shmid;

int * shared_mem(ll n)
{
    key_t key = IPC_PRIVATE; 
    size_t shm_size = sizeof(int)*(n+5); 
    int *arr; 

    if ((shmid = shmget(key, shm_size, IPC_CREAT | 0666)) < 0) 
    { 
        perror("shmget"); _exit(1); 
    }
    
    if ((arr = shmat(shmid, NULL, 0)) == (int *) -1) 
    { 
        perror("shmat"); _exit(1);
    } 

    for(int i=0; i <n; i++) 
        scanf("%d", &arr[i]);

    return arr;
}

void detach(int *arr)
{
    if (shmdt(arr) == -1) 
    { 
        perror("shmdt"); _exit(1); 
    }

    if (shmctl(shmid, IPC_RMID, NULL) == -1) 
    { 
        perror("shmctl"); _exit(1); 
    } 
}

int swap(int *arr, int i, int j)
{
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

int partition (int *arr, int l, int r)  
{  
    srand(time(NULL)); 
    int random = l + rand() % (r - l); 
    swap(arr, random, r); 
    int pivot = arr[r], i = l - 1, temp;

    for (int j = l; j <= r-1; j++)  
    {   
        if (arr[j] < pivot)  
        {  
            i++;  
            swap(arr, i , j);
        }  
    }  

    swap(arr, i+1, r);
    return (i + 1);  
} 

void insertion_sort(int *arr, int l, int r)
{
    for(int i=l+1; i<=r; i++)
    {
        int key = arr[i];
        int j = i-1;
        while(j >= l && arr[j] > key)
        {
            arr[j+1] = arr[j];
            j--;
        }

        arr[j+1] = key;
    }

    return;
}

void quicksort(int *arr, int l, int r)
{
    if(l >= r)
        return;

    if(r - l +1 <= 5)
    {
        insertion_sort(arr, l, r);
        return;
    }

    int pi = partition(arr, l, r);
    quicksort(arr, l, pi - 1);  
    quicksort(arr, pi + 1, r);

    return;
}

void quicksort_proc(int *arr, int l, int r)
{
    if(l > r)
        _exit(1);
    
    if(r - l +1 <= 5)
    {
        insertion_sort(arr, l, r);
        return;
    }

    int pi = partition(arr, l, r);
    pid_t lpid,rpid; 
    lpid = fork();

    if(lpid == 0)
    {
        quicksort_proc(arr, l, pi-1);
        _exit(1);
    }

    else
    {
        rpid = fork();
        if(rpid == 0)
        {
            quicksort_proc(arr, pi+1, r);
            _exit(1);
        }

        else
        {
            int status;
            waitpid(lpid, &status, 0);
            waitpid(rpid, &status, 0);
        }
    
    }

    return;
}

struct arg
{
    int* arr;  
    int l, r;  
};

void *quicksort_threads(void *a)
{
    struct arg *info = (struct arg*) a;
    int *arr = info -> arr;
    int l = info->l, r = info ->r;

    if(l > r)   
        return NULL;

    if(r-l+1 <= 5)
    {
        insertion_sort(arr, l, r);
        return NULL;
    }

    int pi = partition(arr, l, r);
    struct arg a1;
    a1.l = l, a1.r = pi-1, a1.arr = arr; 
    pthread_t l_tid;
    pthread_create(&l_tid, NULL,quicksort_threads, &a1);

    struct arg a2;
    a2.l = pi+1, a2.r = r, a2.arr = arr;
    pthread_t r_tid;
    pthread_create(&r_tid, NULL, quicksort_threads, &a2);

    pthread_join(l_tid, NULL);
    pthread_join(r_tid, NULL);
}

int main()
{
    ll n; scanf("%lld", &n);
    int *arr = shared_mem(n);
    int *brr = (int*) malloc((n+5) * sizeof(int));
    int *crr = (int*) malloc((n+5) * sizeof(int));
    
    for(int i=0; i < n; i++)
        brr[i] = arr[i], crr[i] = arr[i];

    struct timespec ts;
    pthread_t tid;
    struct arg a; a.l = 0, a.r = n-1, a.arr = brr;

    long double st, en, t1, t2, t3;


    //printf("Running Normal QuickSort for n = %lld\n", n);
    
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;

    quicksort(crr, 0, n-1);
    
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    t1 = en - st;
    printf("Time taken by Normal QuickSort = %Lf\n", t1);



    //printf("Running Concurrent QuickSort for n = %lld\n", n);

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;

    quicksort_proc(arr, 0, n-1);

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    t2 = en-st;
    printf("Time taken by Concurrent QuickSort = %Lf\n", t2);
    

    //printf("Running Threaded Concurrent QuickSort for n = %lld\n", n);

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;
    
    pthread_create(&tid, NULL, quicksort_threads, &a);
    pthread_join(tid, NULL);

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    t3 = en-st;
    printf("Time taken by Threaded Concurrent QuickSort = %Lf\n", t3);


    detach(arr);
    free(brr);
    free(crr);
    printf("\nNormal QuickSort is :\n\n%Lf times faster than Concurrent QuickSort\n%Lf times faster than Threaded Concurrent QuickSort\n\n\n", t2/t1, t3/t1);
    
    return 0;
    
}