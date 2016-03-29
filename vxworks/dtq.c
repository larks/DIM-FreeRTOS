/*
 * DTQ (Delphi Timer Queue) implements the action scheduling for the DIM
 * (Delphi Information Managment) System.
 * It will be used by servers clients and the Name Server.
 *
 * Started date   : 10-11-91
 * Written by     : C. Gaspar
 * UNIX adjustment: G.C. Ballintijn
 */

/* include files */
#include <signal.h>
#include <stdio.h>
#include <dim.h>
#ifdef VxWorks
#include <time.h>
#endif

/* global definitions */
#define MAX_TIMER_QUEUES	16	/* Number of normal queue's     */
#define SPECIAL_QUEUE		16	/* The queue for the queue-less */

_PROTO( static int ins_entry,          (int queue_id, TIMR_ENT *entry) );
_PROTO( static int rem_entry,          (int queue_id, TIMR_ENT *entry) );
_PROTO( static int rem_all_entries,    (int queue_id) );
_PROTO( static int scan_timer_list,    (int queue_id, TIMR_ENT *done_entries) );
_PROTO( static void ast_rout,          (int num) );
_PROTO( static void Std_timer_handler, () );


typedef struct {
	TIMR_ENT *queue_head;
	int remove_entries;
} QUEUE_ENT;


static QUEUE_ENT timer_queues[MAX_TIMER_QUEUES + 1] = { 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 
};

static int Inside_ast = 0;
static int alarm_runs = FALSE;
static int curr_time = 0;
static int DTQ_Initialized = FALSE;
static int count = 1;
#ifdef VxWorks
static timer_t Timer_id;
#endif

/*
 * DTQ routines
 */

#define VX_TASK_PRIO 150
#define VX_TASK_SSIZE 20000

int dtq_init()
{
  int tid;
  void timr_handler_task();

  if(!DTQ_Initialized)
    {
      if(tid = taskSpawn("DIM_TIMR_svr", VX_TASK_PRIO, 0, VX_TASK_SSIZE, 
		     timr_handler_task, 0) == ERROR)
      {
         perror("Task Spawn");
      }
      DTQ_Initialized = TRUE;
    }
  return(tid);
}

void timr_handler_task()
{

  struct sigaction sig_info;
  sigset_t set;

     
    sigemptyset(&set);
    sigaddset(&set,SIGIO);
    sig_info.sa_handler = ast_rout;
    sig_info.sa_mask = set;
    sig_info.sa_flags = 0;
	  
    if( sigaction(SIGALRM, &sig_info, 0) < 0 ) {
		perror( "sigaction(SIGALRM)" );
		exit(0);
    }
    timer_create(CLOCK_REALTIME, 0, &Timer_id);
    while(1)
    {
      pause();
    }
}

int alarm(secs)
int secs;
{
	struct itimerspec val, oval;

	val.it_value.tv_sec = secs;
	val.it_value.tv_nsec = 0;
	val.it_interval.tv_sec = 0;
	val.it_interval.tv_nsec = 0;
	timer_settime(Timer_id, 0, &val, &oval);
	return(oval.it_value.tv_sec);
}

int dtq_create()
{
	int i;

	dtq_init();
	for( i = 1; i < MAX_TIMER_QUEUES; i++ )
		if( timer_queues[i].queue_head == 0 )
			break;

	if( i == MAX_TIMER_QUEUES )
		return(0);

	timer_queues[i].queue_head = (TIMR_ENT *)malloc( sizeof(TIMR_ENT) );
	memset( timer_queues[i].queue_head, 0, sizeof(TIMR_ENT) );

	return(i);
}


int dtq_delete(queue_id)
int queue_id;
{
	TIMR_ENT *queue_head;

	if( (queue_head = timer_queues[queue_id].queue_head) != 0 ) {
		rem_all_entries( queue_id );
		free( queue_head );
		timer_queues[queue_id].queue_head = 0;
	}

	return(1);			
}
	
TIMR_ENT *dtq_add_entry(queue_id, time, user_routine, tag)
int queue_id, time, tag;
void (*user_routine)();
{
	TIMR_ENT *new_entry;
	int old_time, new_time;

	DISABLE_AST 
	  /*
	  printf("Adding entry for queue %d - %d secs, tag = %d\n",queue_id, time, tag);
	  */
	if(alarm_runs)
	{
	  if(curr_time > time)
	    {
		curr_time -= alarm(0);
		ast_rout(0);
	    }
	}
	new_entry = (TIMR_ENT *)malloc( sizeof(TIMR_ENT) );
	new_entry->time = time;
    	if( user_routine )
   	   	new_entry->user_routine = user_routine;
	else
       		new_entry->user_routine = Std_timer_handler;
	new_entry->tag = tag;
	ins_entry( queue_id, new_entry );
	if(!alarm_runs)
	{
	  curr_time = 0;
	  ast_rout(1);
	}
	ENABLE_AST

	return(new_entry); 
}


int dtq_clear_entry(entry)
TIMR_ENT *entry;
{
	DISABLE_AST
	  /*
	  printf("Clearing entry for %d secs\n",entry->time);
	  */
	if(alarm_runs)
	{
       		curr_time -= alarm(0);
		ast_rout(0);
	   
	}
	entry->time_left = entry->time;
	if(!alarm_runs)
	{
	  curr_time = 0;
	  ast_rout(1);
	}
	ENABLE_AST
}


int dtq_rem_entry(queue_id, entry)
int queue_id;
TIMR_ENT *entry;
{
	int ret;
	DISABLE_AST
	  /*
	  printf("Removing entry for queue %d, time = %d\n",queue_id, 
		 entry->time);
		 */  
	rem_entry(queue_id, entry);
	ENABLE_AST
	return(1);
}


static int ins_entry(queue_id, entry)
int queue_id;
TIMR_ENT *entry;
{
	TIMR_ENT *auxp, *prevp, *queue_head;
	int entry_time;
	int ins_code = 0;

	queue_head = timer_queues[queue_id].queue_head;
	entry_time = entry->time;
	auxp = queue_head;
	if(auxp->next) {
		do {
			prevp = auxp;
			auxp = auxp->next;
			if(entry_time < auxp->time) {
				auxp = prevp;
				break;
			}
		} while(auxp->next);
	}
	else
		ins_code = 1;
	entry->next = auxp->next;
	auxp->next = entry;
	entry->time_left = entry_time;
	if(auxp->time == entry->time)
		entry->time_left = auxp->time_left;
	return(ins_code);
}


static int rem_entry(queue_id, entry)
int queue_id;
TIMR_ENT *entry;
{
	TIMR_ENT *auxp, *prevp, *queue_head;
	int found = 0;

	if( Inside_ast ) {
		timer_queues[queue_id].remove_entries = 1;
		entry->time = 0;
		return(0);
	}
	queue_head = timer_queues[queue_id].queue_head;
	prevp = auxp = queue_head;
	if(auxp->next) {
		do {
			prevp = auxp;
			auxp = auxp->next;
			if(entry == auxp) {
				found = 1;
				break;
			}
		} while(auxp->next);
	}
	if(found) {
		prevp->next = auxp->next;
		free(auxp);


	}
	return(1);
}


static int rem_all_entries(queue_id)
int queue_id;
{
	TIMR_ENT *auxp, *queue_head;

	DISABLE_AST
	queue_head = timer_queues[queue_id].queue_head;
	auxp = queue_head->next;
	while(auxp) {
		free(auxp);
		auxp = auxp->next;
	}
	ENABLE_AST
}


static int rem_deleted_entries(queue_id)
int queue_id;
{
	TIMR_ENT *auxp, *prevp, *queue_head;

	queue_head = timer_queues[queue_id].queue_head;
	prevp = auxp = queue_head;
	if(auxp->next) {
		do {
			prevp = auxp;
			auxp = auxp->next;
			if(!auxp->time) {
			  /*
			  printf("Removing entry, tag = %d\n",auxp->tag);
			  */
				prevp->next = auxp->next;
				free(auxp);
				auxp = prevp;
			}
		} while(auxp->next);
	}
}


static int scan_all_timer_lists(done_entries, min_time)
TIMR_ENT *done_entries;
int *min_time;
{
	TIMR_ENT *auxp, *aux_donep, *queue_head;
	int queue_id, entries = FALSE;

	*min_time = 100000;
	done_entries->next_done = NULL;
	aux_donep = done_entries;
	for( queue_id = 0; queue_id < MAX_TIMER_QUEUES + 1; queue_id++ ) {
		if( (queue_head = timer_queues[queue_id].queue_head) == NULL )
			continue;
		else if( !(auxp = queue_head->next) )
			continue;
		/*
		printf("Scanning queues, curr_time = %d\n",curr_time);
		*/
		while( auxp ) {
			auxp->time_left -= curr_time;
			if(auxp->time_left <= 0) {
			  /*
	    printf("Timer fired - queue %d, time was %d, tag = %d\n",
	    queue_id, auxp->time, auxp->tag);
				*/
				auxp->next_done = NULL;
				aux_donep->next_done = auxp;
				aux_donep = aux_donep->next_done;
				auxp->time_left = auxp->time; /*restart clock*/
				if( queue_id == SPECIAL_QUEUE )
				{
					dtq_rem_entry(SPECIAL_QUEUE, auxp);
					auxp->time_left = 100000;
				}
				entries = TRUE;
			}
			if(auxp->time_left < *min_time)
				*min_time = auxp->time_left;
			auxp = auxp->next;
		}
	}
	if(*min_time == 100000)
		*min_time = 0;

	return(entries);
}


static void ast_rout( num )
int num;
{
	TIMR_ENT tmout_entries, *aux_donep;
	int queue_id, new_time;

       	Inside_ast++;
	/*
	printf("Inside AST, inside = %d\n",Inside_ast);
	*/
	alarm_runs = FALSE;	
	if( scan_all_timer_lists(&tmout_entries,&new_time) ) {
		aux_donep = tmout_entries.next_done;
		while(aux_donep) {
			aux_donep->user_routine( aux_donep->tag );
			aux_donep = aux_donep->next_done;
		}
	}

	if(Inside_ast == 1)
	  {
	    for( queue_id = 0; queue_id < MAX_TIMER_QUEUES + 1; queue_id++ )
		if( timer_queues[queue_id].remove_entries ) {
			rem_deleted_entries( queue_id );
			timer_queues[queue_id].remove_entries = 0;
		}
	  }
	if(num)
	{
	    if(new_time)
	    {
	      if(alarm_runs)
		{
		  Inside_ast--;
		  /*		  
		  printf("Outside AST 1, inside = %d\n", Inside_ast);
		  */
		  return;
		}
			alarm_runs = TRUE;
			/*
			printf("starting alarm for %d secs\n",new_time);
			*/
			alarm(new_time);
			curr_time = new_time;
	    }
	}
	Inside_ast--;
	/*
	printf("Outside AST, inside = %d\n", Inside_ast);
	*/
}
	

static void Std_timer_handler()
{
}


void dtq_start_timer(time, user_routine, tag)
int time, tag;
void (*user_routine)();
{
	dtq_init();
	if( timer_queues[SPECIAL_QUEUE].queue_head == NULL ) {
		timer_queues[SPECIAL_QUEUE].queue_head = (TIMR_ENT *)malloc(sizeof(TIMR_ENT));
		memset(timer_queues[SPECIAL_QUEUE].queue_head, 0, sizeof(TIMR_ENT));
	}
	(void) dtq_add_entry(SPECIAL_QUEUE, time, user_routine, tag);
}


void dtq_stop_timer(tag)
int tag;
{
	TIMR_ENT *entry;

	for( entry = timer_queues[SPECIAL_QUEUE].queue_head->next;
	     entry != NULL; entry = entry->next )
	{
		if( entry->tag == tag ) {
			dtq_rem_entry( SPECIAL_QUEUE, entry );
			return;
		}
	}
}

#ifndef VxWorks
void dtq_sleep(time_s)
int time_s;
{

	do{
		time_s = sleep(time_s);
	}while(time_s);
}
#endif
