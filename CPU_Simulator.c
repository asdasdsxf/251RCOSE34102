#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PROCESSES  5
#define MAX_QUEUE_PROCESSES 100
#define MAX_IO_EVENTS 3
#define RR_TIME_QUANTUM 2

//IO
typedef struct {
    int time;     
    int duration;

} IOEvent;

//프로세스
typedef struct {
    int pid;
    int arrival_time;
    int burst_time;
    int remaining_time;
    int priority;
    int start_time;
    int completion_time;
    int turnaround_time;
    int waiting_time;

    IOEvent io_events[MAX_IO_EVENTS];
    int io_event_count; 
    int current_io_index;
    int executed_time;    
    int io_remaining_time;    

} Process;


Process processes[MAX_PROCESSES];
int n;

//간트 차트
typedef struct {
    int time;
    int pid;
} GanttEntry;

GanttEntry gantt[100];
int gantt_len = 0;

void record_gantt(int time, Process* p) {
    gantt[gantt_len].time = time;
    if (p != NULL) {
	gantt[gantt_len].pid = p->pid;
	}
    else {
	gantt[gantt_len].pid = -1;
	}
    gantt_len++;
}

void gantt_reset()
{
	for (int i = 0; i < 100; i++)
	{
    	gantt[i].pid = -1;
    	gantt[i].time = -1;
	}
}

//큐 혹은 배열
Process* ready_queue[MAX_QUEUE_PROCESSES];
int front = 0;
int back = 0;
int len = 0;

void enqueue(Process* p) {
    if ((back + 1) % MAX_QUEUE_PROCESSES == front) {
        printf("Ready Queue is Full\n");
        return;
    }
    ready_queue[back] = p;
    back = (back + 1) % MAX_QUEUE_PROCESSES;
}

Process* dequeue() {
    if (front == back) {
        return NULL; // 큐가 비어있음
    }
    Process* p = ready_queue[front];
    front = (front + 1) % MAX_QUEUE_PROCESSES;
    return p;
}

Process* io_queue[MAX_QUEUE_PROCESSES];
int io_front = 0;
int io_back = 0;

void io_enqueue(Process* p) {
    if ((io_back + 1) % MAX_QUEUE_PROCESSES == io_front) {
        printf("IO Queue is Full\n");
        return;
    }
    io_queue[io_back] = p;
    io_back = (io_back + 1) % MAX_QUEUE_PROCESSES;
}

Process* io_dequeue() {
    if (io_front == io_back) {
        return NULL; 
    }
    Process* p = io_queue[io_front];
    io_front = (io_front + 1) % MAX_QUEUE_PROCESSES;
    return p;
}

void queue_reset()
{
	for (int i = 0; i < MAX_QUEUE_PROCESSES; i++)
	{
		ready_queue[i] = NULL;
        io_queue[i] = NULL;
	}
    for (int i = 0; i < MAX_IO_EVENTS; i++)
	{
        io_queue[i] = NULL;
	}
	front = 0;
	back = 0;
	len = 0;
    io_front = 0;
	io_back = 0;
}

// 다음 알고리즘을 위한 프로세스 초기화
void process_reset()
{
	for (int i = 0; i < n; i++) {
        processes[i].remaining_time = processes[i].burst_time;
        processes[i].start_time = -1;
        processes[i].completion_time = -1;
        processes[i].turnaround_time = -1;
        processes[i].waiting_time = -1;

        processes[i].current_io_index = 0;
        processes[i].executed_time = 0;    
        processes[i].io_remaining_time = 0; 


    }
}
//랜덤 값 생성
int rand_int(int min, int max) {
    return rand() % (max - min + 1) + min;
}

//프로세스 생성
void create_processes(int num) {
    n = num;
    int duplication;
    IOEvent temp;
    for (int i = 0; i < n; i++) {
        processes[i].pid = i;
        processes[i].arrival_time = rand_int(0, 10);
        processes[i].burst_time = rand_int(3, 8);
        processes[i].remaining_time = processes[i].burst_time;
        processes[i].priority = rand_int(1, 5);
        processes[i].start_time = -1;
        processes[i].completion_time = -1;
        processes[i].turnaround_time = -1;
        processes[i].waiting_time = -1;

        processes[i].io_event_count = rand_int(0, MAX_IO_EVENTS);       
        processes[i].current_io_index = 0;
        processes[i].executed_time = 0;    
        processes[i].io_remaining_time = 0;

        for (int  j = 0; j < processes[i].io_event_count; j++)
        {
            duplication = 1;
            while(duplication == 1)
            {
                duplication = 0;
                processes[i].io_events[j].time = rand_int(1, processes[i].burst_time-1);
                processes[i].io_events[j].duration = rand_int(1,3);
                for (int k = 0; k < j; k++)
                {
                    if (processes[i].io_events[k].time == processes[i].io_events[j].time)
                    {
                        duplication = 1;
                        processes[i].io_events[j].time = rand_int(1, processes[i].burst_time-1);
                        processes[i].io_events[j].duration = rand_int(1,3);
                    }
                }
            }
        }
        for (int j = 1; j < processes[i].io_event_count; j++) {
            for (int k = 0; k < processes[i].io_event_count - j; k++) {
                if (processes[i].io_events[k].time > processes[i].io_events[k+1].time) {
                    temp = processes[i].io_events[k];
                    processes[i].io_events[k] = processes[i].io_events[k+1];
                    processes[i].io_events[k+1] = temp;
                }
            }   
        }
    }
}

//알고리즘
void FCFS(Process processes[]) {
	queue_reset();
	process_reset();
	Process* current = NULL;
    Process* io_current = NULL;
	gantt_len = 0;
	gantt_reset();
	int end_processes = 0;
	int time = 0;
	while (end_processes < n)
	{
		for (int i = 0; i < n; i++)
		{
			if (processes[i].arrival_time == time)
			{
				enqueue(&processes[i]);
			}
		}
		
		if (current == NULL && front != back) {
            current = dequeue();
            if (current->start_time == -1) {
            current->start_time = time;
            }
        }

        record_gantt(time, current);

        if (io_current == NULL && io_front != io_back) {
            io_current = io_dequeue();
        }

        if (io_current != NULL) {
            io_current->io_remaining_time--;
            if (io_current->io_remaining_time == 0) {
                io_current->current_io_index++;
                enqueue(io_current);
                io_current = NULL;
            }
        }
		
		if (current != NULL) {
            current->remaining_time--;
            current->executed_time++;
            if (current->remaining_time == 0) {
                current->completion_time = time + 1;
                current->turnaround_time = current->completion_time - current->arrival_time;
                current->waiting_time = current->turnaround_time - current->burst_time;
                current = NULL;
                end_processes++;
            }
            else if (current->current_io_index < current->io_event_count && current->executed_time == current->io_events[current->current_io_index].time) {
                io_enqueue(current);
                current->io_remaining_time = current->io_events[current->current_io_index].duration;
                current = NULL;
            }
        }
		
		time ++;
	}
	
}

void Non_Preemptive_SJF(Process processes[]) {
	queue_reset();
	process_reset();
	Process* current = NULL;
    Process* io_current = NULL;
    Process* temp = NULL;
	gantt_len = 0;
	gantt_reset();
	int end_processes = 0;
	int time = 0;

	while (end_processes < n)
	{
		

        for (int i = 0; i < n; i++)
		{
			if (processes[i].arrival_time == time)
			{
				ready_queue[len] = &processes[i];
                len++;
			}
		}
		
        
        if (current == NULL) {
            for (int i = 1; i < len; i++)
            {
                for (int j = 0; j < len - i; j++)
                {
                    if (ready_queue[j]->remaining_time > ready_queue[j+1]->remaining_time)
                    {
                        temp = ready_queue[j];
                        ready_queue[j] = ready_queue[j+1];
                        ready_queue[j+1] = temp;
                    }
                }
            }
        }

        
		if (current == NULL && len != 0) {
            current = ready_queue[0];
            if (current->start_time == -1) {
            current->start_time = time;
            }

            for (int i = 0; i < len - 1; i++)
            {
                ready_queue[i] = ready_queue[i + 1];
            }
            len--;
        }
        record_gantt(time, current);

        if (io_current == NULL && io_front != io_back) {
            io_current = io_dequeue();
        }

        if (io_current != NULL) {
            io_current->io_remaining_time--;
            if (io_current->io_remaining_time == 0) {
                io_current->current_io_index++;
                ready_queue[len] = io_current;
                len++;
                io_current = NULL;
            }
        }

        if (current != NULL) {
            current->remaining_time--;
            current->executed_time++;
            if (current->remaining_time == 0) {
                current->completion_time = time + 1;
                current->turnaround_time = current->completion_time - current->arrival_time;
                current->waiting_time = current->turnaround_time - current->burst_time;
                current = NULL;
                end_processes++;
            }
            else if (current->current_io_index < current->io_event_count && current->executed_time == current->io_events[current->current_io_index].time) {
                io_enqueue(current);
                current->io_remaining_time = current->io_events[current->current_io_index].duration;
                current = NULL;
            }

        }
        
        time++;
    }
}

void Preemptive_SJF(Process processes[]) {
	queue_reset();
	process_reset();
	Process* current = NULL;
    Process* io_current = NULL;
    Process* temp = NULL;
	gantt_len = 0;
	gantt_reset();
	int end_processes = 0;
	int time = 0;

	while (end_processes < n)
	{
		

        for (int i = 0; i < n; i++)
		{
			if (processes[i].arrival_time == time)
			{
				ready_queue[len] = &processes[i];
                len++;
			}
		}
		
        for (int i = 1; i < len; i++)
        {
            for (int j = 0; j < len - i; j++)
            {
                if (ready_queue[j]->remaining_time > ready_queue[j+1]->remaining_time)
                {
                    temp = ready_queue[j];
                    ready_queue[j] = ready_queue[j+1];
                    ready_queue[j+1] = temp;
                }
            }
        }


        
        if (len > 0)
        {
            current = ready_queue[0];
            if (current->start_time == -1) 
            {
                current->start_time = time;
            }
        }
		record_gantt(time, current);

        if (io_current == NULL && io_front != io_back) {
            io_current = io_dequeue();
        }

        if (io_current != NULL) {
            io_current->io_remaining_time--;
            if (io_current->io_remaining_time == 0) {
                io_current->current_io_index++;
                ready_queue[len] = io_current;
                len++;
                io_current = NULL;
            }
        }		
        
		if (current != NULL) {
            current->remaining_time--;
            current->executed_time++;
            if (current->remaining_time == 0) {
                current->completion_time = time + 1;
                current->turnaround_time = current->completion_time - current->arrival_time;
                current->waiting_time = current->turnaround_time - current->burst_time;
                current = NULL;
                end_processes++;
                for (int i = 0; i < len - 1; i++)
                {
                    ready_queue[i] = ready_queue[i + 1];
                }
                len--;
            }
            else if (current->current_io_index < current->io_event_count && current->executed_time == current->io_events[current->current_io_index].time) {
                io_enqueue(current);
                current->io_remaining_time = current->io_events[current->current_io_index].duration;
                for (int i = 0; i < len - 1; i++)
                {
                    ready_queue[i] = ready_queue[i + 1];
                }
                len--;
                current = NULL;
            }
        }

		time ++;
	}
	
}

void Non_Preemptive_Priority(Process processes[]) {
	queue_reset();
	process_reset();
	Process* current = NULL;
    Process* io_current = NULL;
    Process* temp = NULL;
	gantt_len = 0;
	gantt_reset();
	int end_processes = 0;
	int time = 0;

	while (end_processes < n)
	{
		

        for (int i = 0; i < n; i++)
		{
			if (processes[i].arrival_time == time)
			{
				ready_queue[len] = &processes[i];
                len++;
			}
		}
		
        if (current == NULL)
        {
            for (int i = 1; i < len; i++)
            {
                for (int j = 0; j < len - i; j++)
                {
                    if (ready_queue[j]->priority > ready_queue[j+1]->priority)
                    {
                        temp = ready_queue[j];
                        ready_queue[j] = ready_queue[j+1];
                        ready_queue[j+1] = temp;
                    }
                }
            }
        }

        
		if (current == NULL && len != 0) {
            current = ready_queue[0];
            current->start_time = time;

            for (int i = 0; i < len - 1; i++)
            {
                ready_queue[i] = ready_queue[i + 1];
            }
            len--;
        }
        record_gantt(time, current);

        
		if (io_current == NULL && io_front != io_back) {
            io_current = io_dequeue();
        }

        if (io_current != NULL) {
            io_current->io_remaining_time--;
            if (io_current->io_remaining_time == 0) {
                io_current->current_io_index++;
                ready_queue[len] = io_current;
                len++;
                io_current = NULL;
            }
        }

        if (current != NULL) {
            current->remaining_time--;
            current->executed_time++;
            if (current->remaining_time == 0) {
                current->completion_time = time + 1;
                current->turnaround_time = current->completion_time - current->arrival_time;
                current->waiting_time = current->turnaround_time - current->burst_time;
                current = NULL;
                end_processes++;
            }
            else if (current->current_io_index < current->io_event_count && current->executed_time == current->io_events[current->current_io_index].time) {
                io_enqueue(current);
                current->io_remaining_time = current->io_events[current->current_io_index].duration;
                current = NULL;
            }
        }

        time++;
    }
}

void Preemptive_Priority(Process processes[]) {
	queue_reset();
	process_reset();
	Process* current = NULL;
    Process* io_current = NULL;
    Process* temp = NULL;
	gantt_len = 0;
	gantt_reset();
	int end_processes = 0;
	int time = 0;

	while (end_processes < n)
	{
		

        for (int i = 0; i < n; i++)
		{
			if (processes[i].arrival_time == time)
			{
				ready_queue[len] = &processes[i];
                len++;
			}
		}
		
        for (int i = 1; i < len; i++)
        {
            for (int j = 0; j < len - i; j++)
            {
                if (ready_queue[j]->priority > ready_queue[j+1]->priority)
                {
                    temp = ready_queue[j];
                    ready_queue[j] = ready_queue[j+1];
                    ready_queue[j+1] = temp;
                }
            }
        }


        
        if (len > 0)
        {
            current = ready_queue[0];
            if (current->start_time == -1) 
            {
                current->start_time = time;
            }
        }
		record_gantt(time, current);

        
		if (io_current == NULL && io_front != io_back) {
            io_current = io_dequeue();
        }

        if (io_current != NULL) {
            io_current->io_remaining_time--;
            if (io_current->io_remaining_time == 0) {
                io_current->current_io_index++;
                ready_queue[len] = io_current;
                len++;
                io_current = NULL;
            }
        }

		if (current != NULL) {
            current->remaining_time--;
            current->executed_time++;
            if (current->remaining_time == 0) {
                current->completion_time = time + 1;
                current->turnaround_time = current->completion_time - current->arrival_time;
                current->waiting_time = current->turnaround_time - current->burst_time;
                current = NULL;
                end_processes++;
                for (int i = 0; i < len - 1; i++)
                {
                    ready_queue[i] = ready_queue[i + 1];
                }
                len--;
            }
            else if (current->current_io_index < current->io_event_count && current->executed_time == current->io_events[current->current_io_index].time) {
                io_enqueue(current);
                current->io_remaining_time = current->io_events[current->current_io_index].duration;
                for (int i = 0; i < len - 1; i++)
                {
                    ready_queue[i] = ready_queue[i + 1];
                }
                len--;
                current = NULL;
            }
        }

		time ++;
	}
	
}

void RR(Process processes[]) {
	queue_reset();
	process_reset();
	Process* current = NULL;
    Process* io_current = NULL;
	gantt_len = 0;
	gantt_reset();
	int end_processes = 0;
	int time = 0;
    int RR_count = 0;
	while (end_processes < n)
	{
		for (int i = 0; i < n; i++)
		{
			if (processes[i].arrival_time == time)
			{
				enqueue(&processes[i]);
			}
		}
		
		if (current == NULL && front != back) {
            current = dequeue();
            RR_count = 0;
            if (current->start_time == -1){
                current->start_time = time;
            }
        }

        record_gantt(time, current);
        
        if (io_current == NULL && io_front != io_back) {
            io_current = io_dequeue();
        }

        if (io_current != NULL) {
            io_current->io_remaining_time--;
            if (io_current->io_remaining_time == 0) {
                io_current->current_io_index++;
                enqueue(io_current);
                io_current = NULL;
            }
        }

		if (current != NULL) {
            current->remaining_time--;
            current->executed_time++;
            RR_count++;
            if (current->remaining_time == 0) {
                current->completion_time = time + 1;
                current->turnaround_time = current->completion_time - current->arrival_time;
                current->waiting_time = current->turnaround_time - current->burst_time;
                current = NULL;
                end_processes++;
            }
            else if (current->current_io_index < current->io_event_count && current->executed_time == current->io_events[current->current_io_index].time) {
                io_enqueue(current);
                current->io_remaining_time = current->io_events[current->current_io_index].duration;
                current = NULL;
            }
            else if (RR_count == RR_TIME_QUANTUM) {
                enqueue(current);
                current = NULL;
            }
            
        }

		time ++;
	}
	
}

//결과 출력
void print_evaluation_chart() {
	float avg_turn_around_time = 0.0;
	float avg_waiting_time = 0.0;
	for (int i = 0; i < n; i++)
	{
		avg_turn_around_time += (float)processes[i].turnaround_time / n;
		avg_waiting_time += (float)processes[i].waiting_time / n;
	}
	printf("AVERAGE TURN AROUND TIME: %.2f\nAVERAGE WAITING TIME: %.2f", avg_turn_around_time, avg_waiting_time);
		
}

void print_gantt_chart() {
    printf("\nGantt Chart:\n");

    printf(" ");
 
    printf("\n|");

    for (int i = 0; i < gantt_len; i++) {
        if (gantt[i].pid == -1)
            printf("IDLE|");
        else
            printf(" P%-2d|", gantt[i].pid);  
    }


    printf("\n ");
  
    printf("\n");

    printf("0");
    for (int i = 1; i <= gantt_len; i++) {
        printf("   %d", i);
    }
    printf("\n\n");
}



int main() {
    srand(time(NULL));
    create_processes(MAX_PROCESSES);

    printf("\n=== CREATED PROCESSES TABLE ===\n");
    printf("PID\tAT\tBT\tPRIORITY\tI/O Events\n");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        printf("P%d\t%d\t%d\t%d\t\t", 
            processes[i].pid, 
            processes[i].arrival_time, 
            processes[i].burst_time, 
            processes[i].priority);

        // I/O 이벤트 출력
        if (processes[i].io_event_count == 0) {
            printf("None");
        } else {
            for (int j = 0; j < processes[i].io_event_count; j++) {
                printf("(%d,%d) ", 
                    processes[i].io_events[j].time, 
                    processes[i].io_events[j].duration);
            }
        }
        printf("\n");
    }

    printf("\n=== FCFS Scheduling ===\n");
    FCFS(processes);
    print_evaluation_chart();
    print_gantt_chart();

    printf("\n=== Non Preemptive SJF Scheduling ===\n");
    Non_Preemptive_SJF(processes);
    print_evaluation_chart();
    print_gantt_chart();

    printf("\n=== Preemptive SJF Scheduling ===\n");
    Preemptive_SJF(processes);
    print_evaluation_chart();
    print_gantt_chart();

    printf("\n=== Non Preemptive Priority Scheduling ===\n");
    Non_Preemptive_Priority(processes);
    print_evaluation_chart();
    print_gantt_chart();

    printf("\n=== Preemptive Priority Scheduling ===\n");
    Preemptive_Priority(processes);
    print_evaluation_chart();
    print_gantt_chart();

    printf("\n=== ROUND ROBIN Scheduling ===\n");
    RR(processes);
    print_evaluation_chart();
    print_gantt_chart();

    return 0;
}
