#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>

/*
 * Some handy constants. Number of philosophers and chopsticks lets us
 * parameterize the number of concurrent threads and shared
 * resources. The maximum thinking and eating periods let us tune
 * relative periods of holding or not holding a resource. The MAX_BUF
 * and column_width constants help with creating output that makes
 * sense..
 */
#define NUM_PHILS                     5
#define NUM_CHOPS             NUM_PHILS
#define MAX_PHIL_THINK_PERIOD      1000
#define MAX_PHIL_EAT_PERIOD         100
#define MAX_BUF                     256
#define STATS_WIDTH                  16
#define COLUMN_WIDTH                 18
#define ACCOUNTING_PERIOD             5
#define ITERATION_LIMIT              10

/*
 * Structure defining a philosopher and any state we need to know
 * about to print out interesting data, or implement different
 * solutions.
 */
typedef struct {
  int            id;           /* Int ID number assigned by
                                  set_table() */
  pthread_cond_t can_eat;      /* Condition var used in a WAITER SOLUTION */
  int            prog;         /* Progress during current main()
                                  accounting period */
  int            prog_total;   /* Total progress across all
                                  sessions  */
  pthread_t      thread;       /* Thread structure for this
                                  philosopher */
} philosopher;

/* GLOBALS */
philosopher Diners[NUM_PHILS];
int         Stop = 0;

/* Each chopstick is shared between two philosophers */
static pthread_mutex_t chopstick[NUM_CHOPS];

/* WAITER SOLUTION uses these data structures */
static pthread_mutex_t waiter;
static int available_chopsticks[NUM_CHOPS];

/*
 * Helper functions for grabbing chopsticks, referencing neighbors.
 * Numbering assumptions: 
 *   Philosophers: 1 -> NUM_PHILS
 *      - Left philosopher is (number + 1) modulo NUM_PHILS
 *      - Right philosopher is (number - 1) modulo NUM_PHILS
 *   Chopsticks:   1 -> NUM_CHOPS (generally equal to NUM_PHILS)
 *      - Left chopstick has same number as philosopher
 *      - Right chopstick is (philosopher number - 1) modulo NUM_CHOPS
 */
philosopher *left_phil (philosopher *p)
{
  return &Diners[(p->id == (NUM_PHILS-1) ? 0 : (p->id)+1)];
}

philosopher *right_phil (philosopher *p)
{
  return &Diners[(p->id == 0 ? (NUM_PHILS-1) : (p->id)-1)];
}

pthread_mutex_t *left_chop (philosopher *p)
{
  return &chopstick[p->id];
}

pthread_mutex_t *right_chop (philosopher *p)
{
  return &chopstick[(p->id == 0 ? NUM_CHOPS-1 : (p->id)-1)];
}

int *left_chop_available (philosopher *p)
{
  return &available_chopsticks[p->id];
}

int *right_chop_available (philosopher *p)
{
  return &available_chopsticks[(p->id == 0 ? NUM_CHOPS-1 : (p->id)-1)];
}

/*
 * Do a small amount of work that we can use to represent a
 * philosopher thinking one thought
 */
void think_one_thought()
{
  int i;
  i = 0;
  i++;
}

/*
 * Do a small amount of work that we can use to represent a
 * philosopher eating one mouthful of food
 */
void eat_one_mouthful()
{
  int i;
  i = 0;
  i++;
}

static int queue[NUM_PHILS];  
static int queue_front = 0; 
static int queue_rear = 0;
static int queue_size = 0;
static pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

static void enqueue(int phil_id) {
    if (queue_size < NUM_PHILS) {
        queue[queue_rear] = phil_id;
        queue_rear = (queue_rear + 1) % NUM_PHILS;
        queue_size++;
    }
}

static int dequeue() {
    int phil_id = -1;
    if (queue_size > 0) {
        phil_id = queue[queue_front];
        queue_front = (queue_front + 1) % NUM_PHILS;
        queue_size--;
    }
    return phil_id;
}
/*
 * Philosopher code which makes each philosopher eat and think for a
 * random period of time.
 */
static void *dp_thread(void *arg)
{
  int          eat_rnd;
  int          i;
  int          id;
  philosopher *me;
  int          think_rnd;

  me = (philosopher *) arg;
  id = me->id;

  /*
   * While the gobal Stop flag is not set, keep thinking and eating
   * like a good Philosopher.
   */
  while (!Stop) {
    think_rnd = (rand() % MAX_PHIL_THINK_PERIOD);
    eat_rnd   = (rand() % MAX_PHIL_EAT_PERIOD);

    for (i = 0; i < think_rnd; i++){
      think_one_thought();
    }

    pthread_mutex_lock(&waiter);
    
    // Only enqueue if not already in queue
    if (queue_size == 0 || queue[queue_front] != id) {
        enqueue(id);
    }
    
    while (queue[queue_front] != id || 
           !(*left_chop_available(me) && *right_chop_available(me))) {
        pthread_cond_wait(&queue_cond, &waiter);
    }

    dequeue();

    *left_chop_available(me) = 0;
    *right_chop_available(me) = 0;
    pthread_mutex_unlock(&waiter);

    pthread_mutex_lock(left_chop(me));
    pthread_mutex_lock(right_chop(me));

    for (i = 0; i < eat_rnd; i++){
      eat_one_mouthful();
    }

    pthread_mutex_unlock(left_chop(me));
    pthread_mutex_unlock(right_chop(me));

    pthread_mutex_lock(&waiter);
    *left_chop_available(me) = 1;
    *right_chop_available(me) = 1;
    
    // Broadcast to all waiting philosophers
    pthread_cond_broadcast(&queue_cond);
    
    pthread_mutex_unlock(&waiter);

    me->prog++;
    me->prog_total++;
  }

  /*
   * Philosopher thread finished, so rejoin parent
   */
  return NULL;
}

/*
 * Set up the table with the correct number of chopsticks and
 * philosophers and initialize everything.
 */
void set_table()
{
  int i;

  /*
   * Initialize mutex used in the WAITER SOLUTION to represent the
   * waiter
   */
  pthread_mutex_init(&waiter, NULL);

  /*
   * Initialize all the Mutexes that represent the chopsticks. The
   * available flags are used in the WAITER SOLUTION.
   */
  for (i = 0; i < NUM_CHOPS; i++) {
    pthread_mutex_init(&chopstick[i], NULL);
    available_chopsticks[i] = 1;
  }

  /*
   * Initialize the ID number, toal and session progress of each
   * philosopher.
   */
  for (i = 0; i < NUM_PHILS; i++) {
    Diners[i].prog = 0;
    Diners[i].prog_total = 0;
    Diners[i].id = i;
  }

  /*
   * Create each phiilosopher thread. Note that the address of its
   * structure is given as the argument to the dp_thread() routine,
   * and the pthread_t thread element of the structure is filled in by
   * the pthread_create() call.
   */
  for (i = 0; i < NUM_PHILS; i++) {
    pthread_create(&(Diners[i].thread), NULL, dp_thread, &Diners[i]);
  }
}

/*
 * Print the progress of all the philosphers.
 */
void print_progress()
{
  int i;
  int j;

  char buf[MAX_BUF];

  /*
   * Print out the progress for the current accounting period and the
   * total for each philosopher thread.
   */
  for (i = 0; i < NUM_PHILS;) {
    /*
     * Print them in groups of 5 across a line, so use the inner loop
     * of j on 5
     */
    for (j = 0; j < 4; j++) {
      if (i == NUM_PHILS) {
        printf("\n");
        goto out;
      }

      /*
       * Print the accounting period and the total for the current
       * philosopher into a buffer as a string, then print the string
       * with a constant width to make things line up in columns for
       * better readability.
       */
      sprintf(buf, "%d/%d", 
              Diners[i].prog, Diners[i].prog_total);
      printf("p%d=%*s   ", i, STATS_WIDTH, buf);
      i++;
    }

    if (i == NUM_PHILS) {
      printf("\n");
      break;
    }

    /*
     * Print the 5th on the line, with a newline to end the line
     */
    sprintf(buf, "%d/%d", 
            Diners[i].prog, Diners[i].prog_total);
    printf("p%d=%*s\n", i, STATS_WIDTH, buf);
    i++;
  }

out:
  /*
   * Add an extra new line for a blank between data for each
   * accounting period.
   */
  printf("\n");
}

int main(int argc, char **argv)
{
  int i;
  int deadlock;
  int iter;

  iter = 0;

  /*
   * Randomly seed the random number generator used to control how
   * long philosophers eat and think.
   */
  srand(time(NULL));

  /*
   * Set the table means create the chopsticks and the philosophers.
   * Print out a header for the periodic updates on Philosopher state.
   */
  set_table();
  printf("\n");
  printf("Dining Philosophers Update every %d seconds\n", ACCOUNTING_PERIOD);
  printf("-------------------------------------------\n");

  do {
    /*
     * Reset the philosophers eating progress to 0. If the
     * philosopher is making progress, the philosopher will
     * increment it.
     */
    for (i = 0; i < NUM_PHILS; i++)
      Diners[i].prog = 0;

    /*
     * Let the philosophers do some thinking and eating over a period
     * of ACCOUNTING_PERIOD seconds, which is a *long* time compared
     * to the time-scale of a philosopher thread, so *some* progress
     * should be made by each in this waiting time, unless deadlock
     * has occurred.
     */
    sleep(ACCOUNTING_PERIOD);

    /*
     * Check for deadlock (i.e. none of the philosophers have
     * made progress in 5 seconds)
     */
    deadlock = 1;
    for (i = 0; i < NUM_PHILS; i++)
      if (Diners[i].prog)
        deadlock = 0;

    /*
     * Print out the philosophers progress
     */
    print_progress();
    iter++;
  } while (!deadlock && iter < ITERATION_LIMIT);

  /*
   * Set the "Stop flag to tell all diners to stop
   */
  Stop = 1;
  if (deadlock) {
    printf ("Deadlock Detected\n");
  } else {
    printf ("Finished without Deadlock\n");
  }

  /*
   * Release all locks so philosophers can exit even if they are
   * deadlocked.
   */
  for (i = 0; i < NUM_CHOPS; i++)
    pthread_mutex_unlock(&chopstick[i]);

  /*
   * Wait for philosophers to finish
   */
  for (i = 0; i < NUM_PHILS; i++)
    pthread_join(Diners[i].thread, NULL);

  return 0;
}

