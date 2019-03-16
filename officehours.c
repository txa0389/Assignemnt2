
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

/*** Constants that define parameters of the simulation ***/

#define MAX_SEATS 3        /* Number of seats in the professor's office */
#define professor_LIMIT 10 /* Number of students the professor can help before he needs a break */
#define MAX_STUDENTS 1000  /* Maximum number of students in the simulation */

#define CLASSA 0
#define CLASSB 1

/* Add your synchronization variables here */
#define TRUE 1
#define FALSE 0
/* number of students allowed to enter the office sequently */
#define MAX_CLASS_SEQUENCE 5

/* number of students waiting from each class to enter the office */
static int classa_waiting = 0, classb_waiting = 0;
/* number of students sequently entered the class */
static int classa_sequence = 0, classb_sequence = 0;
/* flag to indicate that professor already in the office */
static char professor_arrived = FALSE;
/* event to emit if something changed in the office (break finised, student left office)*/
static pthread_cond_t  office_changed_event = PTHREAD_COND_INITIALIZER;
/* mutex to defend critical section */
static pthread_mutex_t office_guard;

/* Basic information about simulation.  They are printed/checked at the end 
 * and in assert statements during execution.
 *
 * You are responsible for maintaining the integrity of these variables in the 
 * code that you develop. 
 */

static int students_in_office;   /* Total numbers of students currently in the office */
static int classa_inoffice;      /* Total numbers of students from class A currently in the office */
static int classb_inoffice;      /* Total numbers of students from class B in the office */
static int students_since_break = 0;


typedef struct 
{
  int arrival_time;  // time between the arrival of this student and the previous student
  int question_time; // time the student needs to spend with the professor
  int student_id;
} student_info;

/* Called at beginning of simulation.  
 * TODO: Create/initialize all synchronization
 * variables and other global variables that you add.
 */
static int initialize(student_info *si, char *filename) 
{
  students_in_office = 0;
  classa_inoffice = 0;
  classb_inoffice = 0;
  students_since_break = 0;

  /* Initialize your synchronization variables (and 
   * other variables you might use) here
   */

  /* Read in the data file and initialize the student array */
  FILE *fp;

  if((fp=fopen(filename, "r")) == NULL) 
  {
    printf("Cannot open input file %s for reading.\n", filename);
    exit(1);
  }

  int i = 0;
  while ( (fscanf(fp, "%d%d\n", &(si[i].arrival_time), &(si[i].question_time))!=EOF) && i < MAX_STUDENTS ) 
  {
    i++;
  }

 fclose(fp);
 return i;
}

/* Code executed by professor to simulate taking a break 
 * You do not need to add anything here.  
 */
static void take_break() 
{
  printf("The professor is taking a break now.\n");
  sleep(5);
  assert( students_in_office == 0 );
  students_since_break = 0;
}

/* Code for the professor thread. This is fully implemented except for synchronization
 * with the students.  See the comments within the function for details.
 */
void *professorthread(void *junk)
{
  printf("The professor arrived and is starting his office hours\n");

  /* Loop while waiting for students to arrive. */
  while (1)
  {
      /* Add code here to handle the student's request.             */
      /* Currently the body of the loop is empty. There's           */
      /* no communication between professor and students, i.e. all  */
      /* students are admitted without regard of the number         */
      /* of available seats, which class a student is in,           */
      /* and whether the professor needs a break. You need to add   */
      /* all of this.                                               */
    pthread_mutex_lock(&office_guard);
    professor_arrived = TRUE;

    /* if nobody in office and limit of students was reached -> take a break*/
    if (!students_in_office && professor_LIMIT <= students_since_break) {
        take_break();
        /* students from both classes are allowed to enter after the break */
        classa_sequence = classb_sequence = 0;
        /* inform students that it is possible to enter the class */
        pthread_cond_broadcast(&office_changed_event);
    }
    pthread_mutex_unlock(&office_guard);
  }

  if (junk) { junk = NULL; }
  pthread_exit(NULL);
}


/* Code executed by a class A student to enter the office.
 * You have to implement this.  Do not delete the assert() statements,
 * but feel free to add your own.
 */
void classa_enter() 
{
  /* Request permission to enter the office.  You might also want to add  */
  /* synchronization for the simulations variables below                  */

    /* defent critical section with mutex */
    pthread_mutex_lock(&office_guard);

    ++classa_waiting;

    /*
     * A student should wait if:
     * - no professor in the office
     * - professor has a break
     * - max number of students in the office
     * - number of A students passed sequently >= MAX_CLASS_SEQUENCE and there is at least one student from B waiting
    */
    while (!professor_arrived ||
           professor_LIMIT <= students_since_break ||
           classb_inoffice != 0 ||
           students_in_office >= MAX_SEATS ||
           (classb_waiting != 0 && classa_sequence >= MAX_CLASS_SEQUENCE)) {

        pthread_cond_wait(&office_changed_event, &office_guard);
    }

    --classa_waiting;
    ++classa_sequence;
    classb_sequence = 0;

    students_in_office += 1;
    students_since_break += 1;
    classa_inoffice += 1;

    pthread_mutex_unlock(&office_guard);
}

/* Code executed by a class B student to enter the office.
 * You have to implement this.  Do not delete the assert() statements,
 * but feel free to add your own.
 */
void classb_enter() 
{
  /* Request permission to enter the office.  You might also want to add  */
  /* synchronization for the simulations variables below                  */

    /* defent critical section with mutex */
    pthread_mutex_lock(&office_guard);

    ++classb_waiting;

    /*
     * B student should wait if:
     * - no professor in the office
     * - professor has a break
     * - max number of students in the office
     * - number of B students passed sequently >= MAX_CLASS_SEQUENCE and there is at least one student from A waiting
    */
    while (!professor_arrived ||
           professor_LIMIT <= students_since_break ||
           classa_inoffice != 0 ||
           students_in_office >= MAX_SEATS ||
           (classa_waiting != 0 && classb_sequence >= MAX_CLASS_SEQUENCE)) {

        pthread_cond_wait(&office_changed_event, &office_guard);
    }

    --classb_waiting;
    ++classb_sequence;
    classa_sequence = 0;

    students_in_office += 1;
    students_since_break += 1;
    classb_inoffice += 1;

    pthread_mutex_unlock(&office_guard);
}

/* Code executed by a student to simulate the time he spends in the office asking questions
 * You do not need to add anything here.  
 */
static void ask_questions(int t) 
{
  sleep(t);
}


/* Code executed by a class A student when leaving the office.
 * You need to implement this.  Do not delete the assert() statements,
 * but feel free to add as many of your own as you like.
 */
static void classa_leave() 
{
    /* defent critical section with mutex */
    pthread_mutex_lock(&office_guard);
    students_in_office -= 1;
    classa_inoffice -= 1;

    /* notify other students that at least one seat is free */
    pthread_cond_broadcast(&office_changed_event);
    pthread_mutex_unlock(&office_guard);
}

/* Code executed by a class B student when leaving the office.
 * You need to implement this.  Do not delete the assert() statements,
 * but feel free to add as many of your own as you like.
 */
static void classb_leave() 
{
    /* defent critical section with mutex */
    pthread_mutex_lock(&office_guard);
    students_in_office -= 1;
    classb_inoffice -= 1;
    /* notify other students that at least one seat is free */
    pthread_cond_broadcast(&office_changed_event);
    pthread_mutex_unlock(&office_guard);
}

/* Main code for class A student threads.  
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */
void* classa_student(void *si) 
{
  student_info *s_info = (student_info*)si;

  /* enter office */
  classa_enter();

  printf("Student %d from class A enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classb_inoffice == 0 );
  
  /* ask questions  --- do not make changes to the 3 lines below*/
  printf("Student %d from class A starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d from class A finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classa_leave();  

  printf("Student %d from class A leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

  pthread_exit(NULL);
}

/* Main code for class B student threads.
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */
void* classb_student(void *si) 
{
  student_info *s_info = (student_info*)si;

  /* enter office */
  classb_enter();

  printf("Student %d from class B enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classa_inoffice == 0 );

  printf("Student %d from class B starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d from class B finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classb_leave();        

  printf("Student %d from class B leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

  pthread_exit(NULL);
}

/* Main function sets up simulation and prints report
 * at the end.
 */
int main(int nargs, char **args) 
{
  int i;
  int result;
  int student_type;
  int num_students;
  void *status;
  pthread_t professor_tid;
  pthread_t student_tid[MAX_STUDENTS];
  student_info s_info[MAX_STUDENTS];

  if (nargs != 2) 
  {
    printf("Usage: officehour <name of inputfile>\n");
    return EINVAL;
  }

  num_students = initialize(s_info, args[1]);
  if (num_students > MAX_STUDENTS || num_students <= 0) 
  {
    printf("Error:  Bad number of student threads. "
           "Maybe there was a problem with your input file?\n");
    return 1;
  }

  printf("Starting officehour simulation with %d students ...\n",
    num_students);

  result = pthread_create(&professor_tid, NULL, professorthread, NULL);

  if (result) 
  {
    printf("officehour:  pthread_create failed for professor: %s\n", strerror(result));
    exit(1);
  }

  for (i=0; i < num_students; i++) 
  {

    s_info[i].student_id = i;
    sleep(s_info[i].arrival_time);
                
    student_type = random() % 2;

    if (student_type == CLASSA)
    {
      result = pthread_create(&student_tid[i], NULL, classa_student, (void *)&s_info[i]);
    }
    else // student_type == CLASSB
    {
      result = pthread_create(&student_tid[i], NULL, classb_student, (void *)&s_info[i]);
    }

    if (result) 
    {
      printf("officehour: thread_fork failed for student %d: %s\n", 
            i, strerror(result));
      exit(1);
    }
  }

  /* wait for all student threads to finish */
  for (i = 0; i < num_students; i++) 
  {
    pthread_join(student_tid[i], &status);
  }

  /* tell the professor to finish. */
  pthread_cancel(professor_tid);

  printf("Office hour simulation done.\n");

  return 0;
}
