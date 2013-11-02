/* Creates N threads, each of which sleeps a different, fixed
   duration, M times.  Records the wake-up order and verifies
   that it is valid. */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"
#include "random.h"

static void test_sleep (int thread_cnt, int iterations);
static void test_prj3(int thread_num,int duration ,char mode, int pri1,int pri2, int pri3, int pri4, int pri5);
void
test_alarm_single (void) 
{
  test_sleep (5, 1);
}

void
test_alarm_multiple (void) 
{
  test_sleep (5, 7);
}


/* Information about the test. */
struct sleep_test 
  {
    int64_t start;              /* Current time at start of test. */
    int iterations;             /* Number of iterations per thread. */

    /* Output. */
    struct lock output_lock;    /* Lock protecting output buffer. */
    int *output_pos;            /* Current position in output buffer. */
  };

/* Information about an individual thread in the test. */
struct sleep_thread 
  {
    struct sleep_test *test;     /* Info shared between all threads. */
    int id;                     /* Sleeper ID. */
    int duration;               /* Number of ticks to sleep. */
    int iterations;             /* Iterations counted so far. */
  };

static void sleeper (void *);

#define EQUAL_PRIORITY 1
#define SKEW_PRIORITY 2
#define SKEW_RANDOM_PRIORITY 3
#define TEST_EFFICIENCY 4

void  test_fairness_equal_5(void){   test_prj3(5,1000 ,EQUAL_PRIORITY,0,0,0,0,0);   }
void  test_fairness_equal_10(void){    test_prj3(10, 1000,EQUAL_PRIORITY,0,0,0,0,0);   }
void  test_fairness_equal_50(void){    test_prj3(50,7500, EQUAL_PRIORITY,0,0,0,0,0);   }

void  test_fairness_skew_a(void){      test_prj3(5,1000, SKEW_PRIORITY,2,4,6,8,10);   }
void  test_fairness_skew_b(void){      test_prj3(5,1000, SKEW_PRIORITY,2,2,2,2,10);   }
void  test_fairness_skew_c(void){      test_prj3(5,1000, SKEW_PRIORITY,6,6,6,6,30);   }
void  test_fairness_skew_random(void){      test_prj3(5,1000, SKEW_RANDOM_PRIORITY,0,0,0,0,0);   }

void test_efficiency_2(void){  test_prj3(2,10000,TEST_EFFICIENCY,0,0,0,0,0); }
void test_efficiency_5(void){  test_prj3(5,10000,TEST_EFFICIENCY,0,0,0,0,0); }
void test_efficiency_10(void){  test_prj3(10,10000,TEST_EFFICIENCY,0,0,0,0,0); }
void test_efficiency_100(void){  test_prj3(100,10000,TEST_EFFICIENCY,0,0,0,0,0); }
void test_efficiency_500(void){  test_prj3(500,10000,TEST_EFFICIENCY,0,0,0,0,0); }
void test_efficiency_1000(void){  test_prj3(1000,10000,TEST_EFFICIENCY,0,0,0,0,0); }

static inline uint32_t get_cycles(void)
{
     uint32_t low,high;
     
     __asm__ __volatile__("rdtsc" : "=a"(low), "=d"(high));
     return low;
}

/*modified from test_sleep()*/
static void test_prj3(int thread_num,int duration ,  char mode, int pri1,int pri2, int pri3, int pri4, int pri5)
{
  ///random_init((unsigned)get_cycles());
  int thread_cnt = thread_num;
  int iterations = 1;
  
  int priority_map[5];

  if(mode == EQUAL_PRIORITY || mode == SKEW_PRIORITY || mode == SKEW_RANDOM_PRIORITY) bPrintThreadAfterDead = true;
  else if (mode == TEST_EFFICIENCY) bPrintScheduleTime = true;
  else fail("UNKNOWN TESTPROGRAM MODE");
  

  if(mode == SKEW_PRIORITY)
  {
     priority_map[0] = pri1;
     priority_map[1] = pri2;
     priority_map[2] = pri3;
     priority_map[3] = pri4;
     priority_map[4] = pri5;
  }


  struct sleep_test test;
  struct sleep_thread *threads;
  struct thread *t;
  int *output, *op;
  int product;
  int i,finger;

  /* This test does not work with the MLFQS. */
  ASSERT (!thread_mlfqs);
 
  /* Allocate memory. */
  threads = malloc (sizeof *threads * thread_cnt);
  output = malloc (sizeof *output * iterations * thread_cnt * 2);
  if (threads == NULL || output == NULL)
    PANIC ("couldn't allocate memory for test");

  /* Initialize test. */
  test.start = timer_ticks () + 100;
  test.iterations = iterations;
  lock_init (&test.output_lock);
  test.output_pos = output;

  t = thread_current();

  //msg ("name : %s  tick : %lld",t->name ,t->weight_cnt);

  /* Start threads. */
  finger = 0;
  ASSERT (output != NULL);
  for (i = 0; i < thread_cnt; i++)
    {
      struct sleep_thread *t = threads + i;
      char name[16];

      t->test = &test;
      t->id = i;
      t->duration = duration;
      t->iterations = 0;

      snprintf (name, sizeof name, "thread %d", i);
      switch(mode){
	case EQUAL_PRIORITY:
		thread_create (name, PRI_DEFAULT, sleeper, t);
		break;
	case SKEW_PRIORITY:
		thread_create (name, priority_map[finger], sleeper, t);
		finger++;
		break;
	case TEST_EFFICIENCY:
	case SKEW_RANDOM_PRIORITY:
		thread_create (name, (random_ulong()%63)+1, sleeper, t);
		break;
	default:
		 fail("UNKNOWN TESTPROGRAM MODE");
     }
		

    }
  
  /* Wait long enough for all the threads to finish. */
 // timer_sleep (100 + thread_cnt * iterations * 1000 + 100);
  timer_sleep(duration + duration/2);

 // msg ("name : %s  tick : %lld",t->name ,t->weight_cnt);


  /* Acquire the output lock in case some rogue thread is still
     running. */
  lock_acquire (&test.output_lock);

  lock_release (&test.output_lock);
  thread_print_stats();
  free (output);
  free (threads);
}


/* Runs THREAD_CNT threads thread sleep ITERATIONS times each. */
static void
test_sleep (int thread_cnt, int iterations) 
{
  struct sleep_test test;
  struct sleep_thread *threads;
  int *output, *op;
  int product;
  int i;

  /* This test does not work with the MLFQS. */
  ASSERT (!thread_mlfqs);

  msg ("Creating %d threads to sleep %d times each.", thread_cnt, iterations);
  msg ("Thread 0 sleeps 10 ticks each time,");
  msg ("thread 1 sleeps 20 ticks each time, and so on.");
  msg ("If successful, product of iteration count and");
  msg ("sleep duration will appear in nondescending order.");

  /* Allocate memory. */
  threads = malloc (sizeof *threads * thread_cnt);
  output = malloc (sizeof *output * iterations * thread_cnt * 2);
  if (threads == NULL || output == NULL)
    PANIC ("couldn't allocate memory for test");

  /* Initialize test. */
  test.start = timer_ticks () + 100;
  test.iterations = iterations;
  lock_init (&test.output_lock);
  test.output_pos = output;

  /* Start threads. */
  ASSERT (output != NULL);
  for (i = 0; i < thread_cnt; i++)
    {
      struct sleep_thread *t = threads + i;
      char name[16];
      
      t->test = &test;
      t->id = i;
      t->duration = (i + 1) * 10;
      t->iterations = 0;

      snprintf (name, sizeof name, "thread %d", i);
      thread_create (name, PRI_DEFAULT, sleeper, t);
    }
  
  /* Wait long enough for all the threads to finish. */
  timer_sleep (100 + thread_cnt * iterations * 10 + 100);

  /* Acquire the output lock in case some rogue thread is still
     running. */
  lock_acquire (&test.output_lock);

  /* Print completion order. */
  product = 0;
  for (op = output; op < test.output_pos; op++) 
    {
      struct sleep_thread *t;
      int new_prod;

      ASSERT (*op >= 0 && *op < thread_cnt);
      t = threads + *op;

      new_prod = ++t->iterations * t->duration;
        
      msg ("thread %d: duration=%d, iteration=%d, product=%d",
           t->id, t->duration, t->iterations, new_prod);
      
      if (new_prod >= product)
        product = new_prod;
      else
        fail ("thread %d woke up out of order (%d > %d)!",
              t->id, product, new_prod);
    }

  /* Verify that we had the proper number of wakeups. */
  for (i = 0; i < thread_cnt; i++)
    if (threads[i].iterations != iterations)
      fail ("thread %d woke up %d times instead of %d",
            i, threads[i].iterations, iterations);
  
  lock_release (&test.output_lock);
  thread_print_stats();
  free (output);
  free (threads);
}

/* Sleeper thread. */
static void
sleeper (void *t_) 
{
  struct sleep_thread *t = t_;
  struct sleep_test *test = t->test;
  int i;

  for (i = 1; i <= test->iterations; i++) 
    {
      int64_t sleep_until = test->start + i * t->duration;
      timer_sleep (sleep_until - timer_ticks ());
      lock_acquire (&test->output_lock);
      *test->output_pos++ = t->id;
      lock_release (&test->output_lock);
    }
}

