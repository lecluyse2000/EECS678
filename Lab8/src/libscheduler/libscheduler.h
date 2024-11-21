/** @file libscheduler.h
 */

#ifndef LIBSCHEDULER_H_
#define LIBSCHEDULER_H_

/**
  Constants which represent the different scheduling algorithms
*/
typedef enum {FCFS = 0, SJF, PSJF, PRI, PPRI, RR} scheme_t;

void  scheduler_start_up               (const int cores, const scheme_t scheme);
int   scheduler_new_job                (const int job_number, const int time, const int running_time, const int priority);
int   scheduler_job_finished           (const int core_id, const int job_number, const int time);
int   scheduler_quantum_expired        (const int core_id, const int time);
float scheduler_average_turnaround_time();
float scheduler_average_waiting_time   ();
float scheduler_average_response_time  ();
void  scheduler_clean_up               ();

void  scheduler_show_queue             ();

#endif /* LIBSCHEDULER_H_ */
