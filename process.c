#define _GNU_SOURCE
#define _USE_GNU
#include <stdio.h>
#include <stdlib.h> 
#include <sys/types.h>
#include <unistd.h>
#include <sched.h>

const int buffer_size = 100;

int main(int argc, char* argv[]){

    char* name = argv[1];
    pid_t pid = getpid();
    int exec_time = atoi(argv[2]);
    char* start_time_s_string = argv[3];
    char* start_time_ns_string = argv[4];
    long start_time_s = atoi(start_time_s_string);
    long start_time_ns = atoi(start_time_ns_string); 


    int i;
    for(i = 0; i < exec_time; i++){
        volatile unsigned long j;
        for(j = 0; j < 1000000UL; j++); 
    }
    char test_string[100];
    long end_time_s;
    long end_time_ns;   
    long end_time = syscall(334,&end_time_s,&end_time_ns);


    char my_buffer[buffer_size];
    sprintf(my_buffer, "[project1] %d %ld.%09ld %ld.%09ld \n", pid, start_time_s, start_time_ns, end_time_s, end_time_ns);
    syscall(335, my_buffer);
    double time_consume = (float)(end_time_s - start_time_s)+(float)(end_time_ns -start_time_ns)/1e9;
    sprintf(test_string,"[project1] %d %ld.%09ld %ld.%09ld %lf\n", pid, start_time_s, start_time_ns, end_time_s, end_time_ns,time_consume);
    perror(test_string);
    return 0;
}
