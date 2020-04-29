#define _GNU_SOURCE
#define _USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>

const int max_proc_name = 32;
const int max_proc = 10;
const int time_for_RR = 500;

int num_proc_now = 0;
int num_proc_finish = 0;
#define FIFO 0
#define RR 1
#define SJF 2
#define PSJF 3
typedef struct process
{
    char *name;
    int ready_time;
    int exec_time;
    int proc_seq_id;
} Process;
// function for Process sorting

int unit_time_proc()
{
  volatile unsigned long i;
  for(i=0;i<1000000UL;i++);  
}

int compare_proc(const void *a, const void *b)
{
    Process *p1 = (Process *)a;
    Process *p2 = (Process *)b;
    if(p1->ready_time < p2->ready_time)
        return -1;
    if(p1->ready_time > p2->ready_time)
        return 1;
    // if ready_time is equal, compare their proc_seq_id
    if(p1->proc_seq_id < p2->proc_seq_id)
        return -1;
    if(p1->proc_seq_id > p2->proc_seq_id)
        return 1;
    return 0;
}
// function for process list
void insert_live_proc(Process **waiting_list, int policy, Process *proc)
{
    num_proc_now++; 
    if(policy == FIFO || policy == RR) // just insert new process to the list
        waiting_list[num_proc_now - 1] = proc;
    if(policy == SJF || policy == PSJF) // need to find appropriate place according to exec_time
    {
        waiting_list[num_proc_now - 1] = proc;
        for(int i = num_proc_now - 1; i > 0 && waiting_list[i]->exec_time < waiting_list[i - 1]->exec_time; i--)
        {
            // new process should be swapped to left
            Process *temp = waiting_list[i];
            waiting_list[i] = waiting_list[i - 1];
            waiting_list[i - 1] = temp;
        }
    }
}
int exec_proc(Process **waiting_list, int policy) // return how long the process should be executed continuously
{
    if(policy == FIFO || policy == SJF) // first process in the waiting_list will be done
    {
        int exec_length = waiting_list[0]->exec_time;
        waiting_list[0]->exec_time = 0;
        waiting_list[0] = NULL;
        num_proc_now--;
        num_proc_finish++;
        for(int i = 1; i <= num_proc_now; i++)
        {
            Process *temp = waiting_list[i];
            waiting_list[i] = waiting_list[i - 1];
            waiting_list[i - 1] = temp;
        }
        return exec_length; 
    }
    if(policy == RR)
    {
        int exec_length;
        if(waiting_list[0]->exec_time > time_for_RR)
        {
            exec_length = time_for_RR;
            waiting_list[0]->exec_time -= time_for_RR;
        }
        else
        {
            exec_length = waiting_list[0]->exec_time;
            waiting_list[0]->exec_time = 0;
            waiting_list[0] = NULL; // this process is done
        }
        for(int i = 1; i < num_proc_now; i++)
        {
            Process *temp = waiting_list[i];
            waiting_list[i] = waiting_list[i - 1];
            waiting_list[i - 1] = temp;
        }
        if(waiting_list[num_proc_now - 1] == NULL)
        {
            num_proc_now--;
            num_proc_finish++;
        }
        return exec_length;
    }
    if(policy == PSJF) // execute only one time unit
    {
        waiting_list[0]->exec_time--;
        if(waiting_list[0]->exec_time == 0)
        {
            waiting_list[0] = NULL;
            num_proc_now--;
            num_proc_finish++;
            for(int i = 1; i <= num_proc_now; i++)
            {
                Process *temp = waiting_list[i];
                waiting_list[i] = waiting_list[i - 1];
                waiting_list[i - 1] = temp;
            }
        }
        return 1;
    }
}
// scheduling main program
int main()
{
    Process proc[max_proc];
    char policy_name[5];
    scanf("%s", policy_name);
    int num_proc;
    scanf("%d", &num_proc);
    for(int i = 0; i < num_proc; i++)
    {
        char *process_name = malloc(max_proc_name * sizeof(char));
        int ready_time;
        int execution_time;
        scanf("%s %d %d", process_name, &ready_time, &execution_time);
        proc[i].name = process_name;
        proc[i].ready_time = ready_time;
        proc[i].exec_time = execution_time;
        proc[i].proc_seq_id = i;
    }
    // sort process according to ready_time
    qsort(proc, num_proc, sizeof(Process), compare_proc);
    int policy = 0;
    char policy_list[4][5] = {"FIFO", "RR", "SJF", "PSJF"};
    for(int i = 0; i < 4; i++)
        if(strcmp(policy_name, policy_list[i]) == 0)
            policy = i;
    Process *waiting_list[max_proc];
    for(int i = 0; i < max_proc; i++)
        waiting_list[i] = NULL;
    
    //set CPU for parent
    
    cpu_set_t cpu_mask;
    CPU_ZERO(&cpu_mask);
    CPU_SET(0, &cpu_mask);
    
    if(sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask) != 0){
        perror("sched_setaffinity error");
        exit(EXIT_FAILURE);
    }

    //raise the priority
    
    const int priority_high = 80;
    const int priority_low = 10;
    //printf("H:%d L:%d\n",priority_high,priority_low); 
    struct sched_param param;
    param.sched_priority = 50;
    //Just want to make sure it won't get preempted by other processes on its CPU
    pid_t pid_parent = getpid();
    if(sched_setscheduler(pid_parent, SCHED_FIFO, &param) != 0) {
        perror("sched_setscheduler error");
        exit(EXIT_FAILURE);  
    }
    
    // Proc already sorted in ascending order of ready_time
    int time_count = 0;
    int fork_count = 0;
    Process *exec_process_now = NULL;
    Process *exec_process_last = NULL;
    int num_proc_finish_last = 0;
    int exec_length = 0;
    pid_t pids[max_proc];
    while(!(num_proc_finish == num_proc && exec_length == 0))
    {
        //fork the processes that are ready at time_count
        long start_time_s;
        long start_time_ns; 
        char test_string[100];      
        long mytime = syscall(334,&start_time_s,&start_time_ns);

        while(proc[fork_count].ready_time <= time_count && fork_count < num_proc)
        {
            sprintf(test_string,"%s : fork at time %d\n", proc[fork_count].name, time_count);
            perror(test_string);
            pid_t pid = fork();
            
            if(pid < 0)   
                printf("error in fork!");   
            else if(pid == 0) {
                // child
                char exec_time[10];
                sprintf(exec_time, "%d", proc[fork_count].exec_time);
                char start_time_s_string[20]; 
                char start_time_ns_string[20];
                sprintf(start_time_s_string, "%ld", start_time_s);
                sprintf(start_time_ns_string, "%ld", start_time_ns);
                if(execlp("./process", "process", proc[fork_count].name, exec_time, start_time_s_string, start_time_ns_string, (char *)NULL) < 0){
                    perror("execlp error");
                    exit(EXIT_FAILURE);
                }
            }  
            //parent
            //change the newly forked child's CPU
            cpu_set_t cpu_mask;
            CPU_ZERO(&cpu_mask);
            CPU_SET(1, &cpu_mask);

            if(sched_setaffinity(pid, sizeof(cpu_set_t), &cpu_mask) != 0){
                perror("sched_setaffinity error");
                exit(EXIT_FAILURE);
            }

            pids[proc[fork_count].proc_seq_id] = pid; // use pid (also 0 ~ num_proc - 1) as index to store pids
            insert_live_proc(waiting_list, policy, (proc + fork_count));
            fork_count++;
        }
        
        // decide next process  
        
        // no process is waiting
        if(waiting_list[0] == NULL && exec_length == 0)
        {
            time_count++;
            
            unit_time_proc(); 
            exec_process_last = NULL;
        }
        else
        {
            // find next process in the list
            if(exec_length == 0)
            {
                exec_process_now = waiting_list[0];
                exec_length = exec_proc(waiting_list, policy);
                //change priority if a different process is going to run
                if(exec_process_last == NULL || exec_process_last->exec_time == 0)
                {
		            //printf("Set %s to high at time %d\n", exec_process_now->name, time_count);
                    pid_t pid = pids[exec_process_now->proc_seq_id];
		    //printf("PID: %d\n",pid);
                    param.sched_priority = priority_high;
                    if(sched_setscheduler(pid, SCHED_RR, &param) != 0) {
                        perror("sched_setscheduler error");
                        exit(EXIT_FAILURE);
                    }
			        //if(sched_getscheduler(pid) == SCHED_RR) puts("RR");
                }
                // recover priority of last process
                else
		        {
		            //if (exec_process_now->name != exec_process_last->name) printf("Set %s to high, recover %s to low at time %d\n", exec_process_now->name, exec_process_last->name, time_count);
                    pid_t pid_last = pids[exec_process_last->proc_seq_id];
                    pid_t pid = pids[exec_process_now->proc_seq_id];
		    //printf("PID:%d\n",pid);
                    param.sched_priority = priority_low;
                    if(sched_setscheduler(pid_last, SCHED_RR, &param) != 0) {
                        perror("sched_setscheduler error");
                        exit(EXIT_FAILURE);  
                    }

                    param.sched_priority = priority_high;
                    if(sched_setscheduler(pid, SCHED_FIFO, &param) != 0) {
                        perror("sched_setscheduler error");
                        exit(EXIT_FAILURE);  
                    }
                }	
            }

            exec_length--;
            time_count++;
            unit_time_proc(); 
            
            // exec time section for this process is over
            if(exec_length == 0)
            {
                // check if the process has terminated, if so, wait for it
                if(num_proc_finish == num_proc_finish_last + 1)
                {
                    //printf("%s : end at time %d\n", exec_process_now->name, time_count);
                    int status;
                    if(waitpid(pids[exec_process_now->proc_seq_id], &status, 0) == -1)
                    {
                        perror("waitpid error");
                        exit(EXIT_FAILURE);
                    }
                }

                exec_process_last = exec_process_now;
                num_proc_finish_last = num_proc_finish;
            }   
        }    
    }

    for(int i = 0; i < num_proc; i++){
        printf("%s ", proc[i].name);
        printf("%d\n", pids[proc[i].proc_seq_id]);
    }

}
