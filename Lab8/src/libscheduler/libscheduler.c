/** @file libscheduler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"


/**
  Stores information making up a job to be scheduled including any statistics.

  You may need to define some global variables or a struct to store your job queue elements. 
*/

typedef struct _completed_job_t {
    int arrival_time;
    int start_time;
    int end_time;
    int first_run_time;
    int total_run_time;
} completed_job_t;

completed_job_t *completed_jobs = NULL;
int completed_jobs_count = 0;

typedef struct _job_t
{
    int job_number;
    int arrival_time;
    int running_time;
    int time_remaining;
    int priority;
    int core_id;
    int start_time;
    int end_time;
    int total_run_time;
    int first_run_time;
    int quantum_used;
} job_t;

priqueue_t job_queue;
int num_cores;
scheme_t scheduler_scheme;
job_t **core_jobs;
int total_jobs;

static int compare_fcfs(const void *const lhs, const void *const rhs)
{
    job_t *job1 = (job_t*)lhs;
    job_t *job2 = (job_t*)rhs;

    return job1->arrival_time - job2->arrival_time;
}

static int compare_sjf(const void *const lhs, const void *const rhs)
{
    job_t *job1 = (job_t*)lhs;
    job_t *job2 = (job_t*)rhs;
    if (job1->running_time == job2->running_time) {
        return job1->arrival_time - job2->arrival_time;
    }

    return job1->running_time - job2->running_time;
}

static int compare_psjf(const void *const lhs, const void *const rhs)
{
    job_t *job1 = (job_t*)lhs;
    job_t *job2 = (job_t*)rhs;
    
    if (job1->time_remaining != job2->time_remaining) {
        return job1->time_remaining - job2->time_remaining;
    }
    
    return job1->arrival_time - job2->arrival_time;
}

static int compare_pri(const void *const lhs, const void *const rhs)
{
    job_t *job1 = (job_t*)lhs;
    job_t *job2 = (job_t*)rhs;
    if (job1->priority == job2->priority) {
        return job1->arrival_time - job2->arrival_time;
    }

    return job1->priority - job2->priority;
}

static int compare_ppri(const void *const lhs, const void *const rhs)
{
    return compare_pri(lhs, rhs);
}

static int compare_rr(const void *const lhs, const void *const rhs)
{
    return 0;
}

/**
  Initalizes the scheduler.
 
  Assumptions:
    - You may assume this will be the first scheduler function called.
    - You may assume this function will be called once once.
    - You may assume that cores is a positive, non-zero number.
    - You may assume that scheme is a valid scheduling scheme.

  @param cores the number of cores that is available by the scheduler. These cores will be known as core(id=0), core(id=1), ..., core(id=cores-1).
  @param scheme  the scheduling scheme that should be used. This value will be one of the six enum values of scheme_t
*/
void scheduler_start_up(const int cores, const scheme_t scheme)
{
    num_cores = cores;
    scheduler_scheme = scheme;
    core_jobs = malloc(sizeof(job_t*) * cores);
    total_jobs = 0;
    completed_jobs_count = 0;
    completed_jobs = NULL;
    
    for (int i = 0; i < cores; i++) {
        core_jobs[i] = NULL;
    }

    switch(scheme) {
        case FCFS: priqueue_init(&job_queue, compare_fcfs); break;
        case SJF:  priqueue_init(&job_queue, compare_sjf);  break;
        case PSJF: priqueue_init(&job_queue, compare_psjf); break;
        case PRI:  priqueue_init(&job_queue, compare_pri);  break;
        case PPRI: priqueue_init(&job_queue, compare_ppri); break;
        case RR:   priqueue_init(&job_queue, compare_rr);   break;
    }
}


/**
  Called when a new job arrives.
 
  If multiple cores are idle, the job should be assigned to the core with the
  lowest id.
  If the job arriving should be scheduled to run during the next
  time cycle, return the zero-based index of the core the job should be
  scheduled on. If another job is already running on the core specified,
  this will preempt the currently running job.
  Assumption:
    - You may assume that every job wil have a unique arrival time.

  @param job_number a globally unique identification number of the job arriving.
  @param time the current time of the simulator.
  @param running_time the total number of time units this job will run before it will be finished.
  @param priority the priority of the job. (The lower the value, the higher the priority.)
  @return index of core job should be scheduled on
  @return -1 if no scheduling changes should be made. 
 
 */

inline static int find_free_core()
{
    for (int i = 0; i < num_cores; i++) {
        if (core_jobs[i] == NULL) return i;
    }
    return -1;
}

int scheduler_new_job(const int job_number, const int time, const int running_time, const int priority)
{
    if (running_time <= 0 || time < 0) {
        return -1;
    }
    
    job_t *job = malloc(sizeof(job_t));
    if (!job) {
        return -1;
    }
    job->job_number = job_number;
    job->arrival_time = time;
    job->running_time = running_time;
    job->time_remaining = running_time;
    job->priority = priority;
    job->core_id = -1;
    job->start_time = time;
    job->end_time = -1;
    job->first_run_time = -1;
    job->quantum_used = 0;
    total_jobs++;

    const int free_core = find_free_core();
    if (free_core != -1) {
        job->core_id = free_core;
        job->first_run_time = time;
        core_jobs[free_core] = job;
        return free_core;
    }

    if (scheduler_scheme == PSJF) {
        // Find job with most remaining time
        int max_remaining = -1;
        int core_to_preempt = -1;
        
        for (int i = 0; i < num_cores; i++) {
            if (core_jobs[i]) {
                int elapsed = time - core_jobs[i]->start_time;
                int remaining = core_jobs[i]->time_remaining - elapsed;
                
                if (remaining > job->time_remaining && 
                    (max_remaining == -1 || remaining > max_remaining)) {
                    max_remaining = remaining;
                    core_to_preempt = i;
                }
            }
        }
        
        if (core_to_preempt != -1) {
            int elapsed = time - core_jobs[core_to_preempt]->start_time;
            core_jobs[core_to_preempt]->time_remaining -= elapsed;
            core_jobs[core_to_preempt]->start_time = time;
            priqueue_offer(&job_queue, core_jobs[core_to_preempt]);
            
            job->core_id = core_to_preempt;
            job->start_time = time;
            job->first_run_time = time;
            core_jobs[core_to_preempt] = job;
            return core_to_preempt;
        }
    }
    else if (scheduler_scheme == PPRI) {
        // Find job with lowest priority
        int highest_priority = -1;
        int core_to_preempt = -1;
        
        for (int i = 0; i < num_cores; i++) {
            if (core_jobs[i] && core_jobs[i]->priority > job->priority) {
                if (highest_priority == -1 || core_jobs[i]->priority > highest_priority) {
                    highest_priority = core_jobs[i]->priority;
                    core_to_preempt = i;
                }
            }
        }
        
        if (core_to_preempt != -1) {
            int elapsed = time - core_jobs[core_to_preempt]->start_time;
            core_jobs[core_to_preempt]->time_remaining -= elapsed;
            core_jobs[core_to_preempt]->start_time = time;
            priqueue_offer(&job_queue, core_jobs[core_to_preempt]);
            
            job->core_id = core_to_preempt;
            job->start_time = time;
            job->first_run_time = time;
            core_jobs[core_to_preempt] = job;
            return core_to_preempt;
        }
    }
    
    priqueue_offer(&job_queue, job);
    return -1;
}

/**
  Called when a job has completed execution.
 
  The core_id, job_number and time parameters are provided for convenience. You may be able to calculate the values with your own data structure.
  If any job should be scheduled to run on the core free'd up by the
  finished job, return the job_number of the job that should be scheduled to
  run on core core_id.
 
  @param core_id the zero-based index of the core where the job was located.
  @param job_number a globally unique identification number of the job.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled to run on core core_id
  @return -1 if core should remain idle.
 */
int scheduler_job_finished(const int core_id, const int job_number, const int time)
{
    if (core_jobs[core_id]) {
        core_jobs[core_id]->end_time = time;

        completed_jobs = realloc(completed_jobs, (completed_jobs_count + 1) * sizeof(completed_job_t));
        completed_jobs[completed_jobs_count].arrival_time = core_jobs[core_id]->arrival_time;
        completed_jobs[completed_jobs_count].first_run_time = core_jobs[core_id]->first_run_time;
        completed_jobs[completed_jobs_count].end_time = time;
        completed_jobs_count++;

        free(core_jobs[core_id]);
        core_jobs[core_id] = NULL;
    }
    
    if (priqueue_size(&job_queue) > 0) {
        job_t *next_job = priqueue_poll(&job_queue);
        core_jobs[core_id] = next_job;
        next_job->core_id = core_id;
        next_job->start_time = time;
        if (next_job->first_run_time == -1) {
            next_job->first_run_time = time;
        }
        return next_job->job_number;
    }
    
    return -1;
}

/**
  When the scheme is set to RR, called when the quantum timer has expired
  on a core.
 
  If any job should be scheduled to run on the core free'd up by
  the quantum expiration, return the job_number of the job that should be
  scheduled to run on core core_id.

  @param core_id the zero-based index of the core where the quantum has expired.
  @param time the current time of the simulator. 
  @return job_number of the job that should be scheduled on core cord_id
  @return -1 if core should remain idle
 */

int scheduler_quantum_expired(const int core_id, const int time)
{
    if (scheduler_scheme != RR) {
        return -1;
    }
    
    job_t *current_job = core_jobs[core_id];
    if (!current_job) {
        if (priqueue_size(&job_queue) > 0) {
            job_t *next_job = priqueue_poll(&job_queue);
            core_jobs[core_id] = next_job;
            next_job->core_id = core_id;
            next_job->start_time = time;
            if (next_job->first_run_time == -1) {
                next_job->first_run_time = time;
            }
            return next_job->job_number;
        }
        return -1;
    }

    const int elapsed = time - current_job->start_time;
    current_job->time_remaining -= elapsed;

    if (current_job->time_remaining <= 0) {
        completed_jobs = realloc(completed_jobs, (completed_jobs_count + 1) * sizeof(completed_job_t));
        completed_jobs[completed_jobs_count].arrival_time = current_job->arrival_time;
        completed_jobs[completed_jobs_count].first_run_time = current_job->first_run_time;
        completed_jobs[completed_jobs_count].end_time = time;
        completed_jobs_count++;
        
        free(core_jobs[core_id]);
        core_jobs[core_id] = NULL;
    } else {
        current_job->start_time = time;
        current_job->core_id = -1;
        priqueue_offer(&job_queue, current_job);
        core_jobs[core_id] = NULL;
    }
    
    if (priqueue_size(&job_queue) > 0) {
        job_t *next_job = priqueue_poll(&job_queue);
        core_jobs[core_id] = next_job;
        next_job->core_id = core_id;
        next_job->start_time = time;
        if (next_job->first_run_time == -1) {
            next_job->first_run_time = time;
        }
        return next_job->job_number;
    }
    
    return -1;
}

/**
  Returns the average waiting time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average waiting time of all jobs scheduled.
 */

float scheduler_average_waiting_time()
{
    if (total_jobs == 0) return 0.0f;
    
    float total_waiting_time = 0.0f;
    for (int i = 0; i < completed_jobs_count; i++) {
        int turnaround_time = completed_jobs[i].end_time - completed_jobs[i].arrival_time;
        int running_time = completed_jobs[i].total_run_time;
        total_waiting_time += (float)(turnaround_time - running_time);
    }
    
    return total_waiting_time / total_jobs;
}

/**
  Returns the average turnaround time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average turnaround time of all jobs scheduled.
 */

float scheduler_average_turnaround_time()
{
    if (total_jobs == 0) return 0.0f;
    
    float total_turnaround_time = 0.0f;
    
    for (int i = 0; i < completed_jobs_count; i++) {
        total_turnaround_time += (float)(completed_jobs[i].end_time - 
                                       completed_jobs[i].arrival_time);
    }
    
    return total_turnaround_time / total_jobs;
}

/**
  Returns the average response time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average response time of all jobs scheduled.
 */

float scheduler_average_response_time()
{
    if (total_jobs == 0) return 0.0f;
    
    float total_response_time = 0.0f;
    for (int i = 0; i < completed_jobs_count; i++) {
        total_response_time += (float)(completed_jobs[i].first_run_time - completed_jobs[i].arrival_time);
    }
    
    return total_response_time / total_jobs;
}

/**
  Free any memory associated with your scheduler.
 
  Assumption:
    - This function will be the last function called in your library.
*/
void scheduler_clean_up()
{
    for (int i = 0; i < num_cores; i++) {
        if (core_jobs[i]) {
            free(core_jobs[i]);
        }
    }

    free(core_jobs);
    free(completed_jobs);
    priqueue_destroy(&job_queue);
}


/**
  This function may print out any debugging information you choose. This
  function will be called by the simulator after every call the simulator
  makes to your scheduler.
  In our provided output, we have implemented this function to list the jobs in the order they are to be scheduled. Furthermore, we have also listed the current state of the job (either running on a given core or idle). For example, if we have a non-preemptive algorithm and job(id=4) has began running, job(id=2) arrives with a higher priority, and job(id=1) arrives with a lower priority, the output in our sample output will be:

    2(-1) 4(0) 1(-1)  
  
  This function is not required and will not be graded. You may leave it
  blank if you do not find it useful.
 */
void scheduler_show_queue()
{
    for (int i = 0; i < priqueue_size(&job_queue); i++) {
        const job_t *const job = priqueue_at(&job_queue, i);
        printf("%d(%d) ", job->job_number, job->core_id);
    }
    printf("\n");
}
