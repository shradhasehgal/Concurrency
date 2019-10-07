#define _POSIX_C_SOURCE 199309L //required for clock
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <math.h>


int * shareMem(size_t size){
    key_t mem_key = IPC_PRIVATE;
    int shm_id = shmget(mem_key, size, IPC_CREAT | 0666);
    return (int*)shmat(shm_id, NULL, 0);
}


void merge(int *arr, int l, int r){
    int mid = (l+r)/2;
    int *sorted = (int*)malloc(sizeof(int)*(r-l+1));
    int j=mid+1, c=0, i=0;

    for(i=l;i<=mid;i++){
        for(;j<=r && arr[j]<=arr[i];j++)
            sorted[c++]=arr[j];
        sorted[c++] = arr[i];
    }

    while(i<=mid) sorted[c++]=arr[i++];
    while(j<=r) sorted[c++]=arr[j++];

    for(i=0;i<c;i++) arr[l+i]=sorted[i];
    free(sorted);
}

void normal_mergesort(int *arr, int l, int r){
    if(l>r) return;

    //insertion sort
    if(r-l+1<=5){
        int a[5], mi=INT_MAX, mid=-1;
        for(int i=l;i<r;i++)
        {
            int j=i+1; 
            for(;j<=r;j++)
                if(arr[j]<arr[i] && j<=r) 
                {
                    int temp = arr[i];
                    arr[i] = arr[j];
                    arr[j] = temp;
                }
        }
        return;
    }

    normal_mergesort(arr, l, (l+r)/2);
    normal_mergesort(arr, (l+r)/2+1, r);
    merge(arr, l, r);
}

void mergesort(int *arr, int l, int r){
    if(l>r) _exit(1);    
    
    //insertion sort
    if(r-l+1<=5){
        int a[5], mi=INT_MAX, mid=-1;
        for(int i=l;i<r;i++)
        {
            int j=i+1; 
            for(;j<=r;j++)
                if(arr[j]<arr[i] && j<=r) 
                {
                    int temp = arr[i];
                    arr[i] = arr[j];
                    arr[j] = temp;
                }
        }
        return;
    } 

    int pid1 = fork();
    int pid2;
    if(pid1==0){
        //sort left half array
        mergesort(arr, l, (l+r)/2);
        _exit(1);
    }
    else{
        pid2 = fork();
        if(pid2==0){
            //sort right half array
            mergesort(arr,(l+r)/2+1,r);
            _exit(1);
        }
        else{
            //wait for the right and the left half to get sorted
            int status;
            waitpid(pid1, &status, 0);
            waitpid(pid2, &status, 0);
        }
    }
    merge(arr, l, r);
    return;
}

struct arg{
    int l;
    int r;
    int* arr;    
};

void *threaded_mergesort(void* a){
    //note that we are passing a struct to the threads for simplicity.
    struct arg *args = (struct arg*) a;

    int l = args->l;
    int r = args->r;
    int *arr = args->arr;
    if(l>r) return NULL;    
    
    //insertion sort
    if(r-l+1<=5){
        int a[5], mi=INT_MAX, mid=-1;
        for(int i=l;i<r;i++)
        {
            int j=i+1; 
            for(;j<=r;j++)            
                if(arr[j]<arr[i] && j<=r) 
                {
                    int temp = arr[i];
                    arr[i] = arr[j];
                    arr[j] = temp;
                }
        }
        return NULL;
    }

    //sort left half array
    struct arg a1;
    a1.l = l;
    a1.r = (l + r)/2;
    a1.arr = arr;
    pthread_t tid1;
    pthread_create(&tid1, NULL, threaded_mergesort, &a1);
    
    //sort right half array
    struct arg a2;
    a2.l = (l+r)/2+1;
    a2.r = r;
    a2.arr = arr;
    pthread_t tid2;
    pthread_create(&tid2, NULL, threaded_mergesort, &a2);
    
    //wait for the two halves to get sorted
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    
    merge(arr,l,r);
}

void runSorts(long long int n){

    struct timespec ts;
    
    //getting shared memory
    int *arr = shareMem(sizeof(int)*(n+1));
    for(int i=0;i<n;i++) scanf("%d", arr+i);


    printf("Running concurrent_mergesort for n = %lld\n", n);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long double st = ts.tv_nsec/(1e9)+ts.tv_sec;

    //multiprocess mergesort
    mergesort(arr, 0, n-1);
    
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long double en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("time = %Lf\n", en - st);
    long double t1 = en-st;

    int brr[n+1];
    for(int i=0;i<n;i++) brr[i] = arr[i];


    pthread_t tid;
    struct arg a;
    a.l = 0;
    a.r = n-1;
    a.arr = brr;
    printf("Running threaded_concurrent_mergesort for n = %lld\n", n);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;

    //multithreaded mergesort
    pthread_create(&tid, NULL, threaded_mergesort, &a);
    pthread_join(tid, NULL);    
    
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("time = %Lf\n", en - st);
    long double t2 = en-st;

    printf("Running normal_mergesort for n = %lld\n", n);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;

    // normal mergesort
    normal_mergesort(brr, 0, n-1);    
    
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("time = %Lf\n", en - st);
    long double t3 = en - st;

    printf("normal_mergesort ran:\n\t[ %Lf ] times faster than concurrent_mergesort\n\t[ %Lf ] times faster than threaded_concurrent_mergesort\n\n\n", t1/t3, t2/t3);
    shmdt(arr);
    return;
}

int main(){

    long long int n;
    scanf("%lld", &n);
    runSorts(n);
    return 0;
}