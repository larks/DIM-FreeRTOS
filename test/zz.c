/*
#define _REENTRANT
*/
#include <stdio.h>
/*
#include <thread.h>
*/
#include <dim.h>
#include <dis.h>

#define NUM_THREADS 5

void *change_global_data();     /*  for thr_create()   */

char str[10][80];
int ids[10];

rout(tag, str, size)
int tag; 
{
  printf("Thread %d updating...\n",tag);
  dis_update_service(ids[tag]);
  dtq_start_timer(5,rout,tag);
}

main(argc, argv)
int argc;
char * argv[];  
{
	char serv_name[80];
	int i=0;
	for (i=0; i< NUM_THREADS; i++)     {
	  /*
		thr_create(NULL, 0, change_global_data, i, 0, NULL);
		*/
	  dtq_start_timer(1,change_global_data,i);
	}
	sleep(1);
	for (i=5; i< 10; i++)     {
	sprintf(str[i],"xx/Service_%03d",i);
	{
	  /*
	DISABLE_AST
	*/

	ids[i] = dis_add_service(str[i],"C",str[i], strlen(str[i])+1, (void *)0, 0);
	dis_start_serving("xx");
	/*
	ENABLE_AST
	*/
	  }
	printf("Thread %d registered service %s\n",i,str[i]);
	dtq_start_timer(5,rout,i);
	}
	/*
	while ((thr_join(NULL, NULL, NULL) == 0));
	*/
	while(1)
	  pause();
}

void * change_global_data(par)
int par;   
{
  /*
	static mutex_t Global_mutex;
	*/
	static int     Global_data = 0;
	char serv_name[80];

	
	/*
	mutex_lock(&Global_mutex);
	*/
	/*
	Global_data++;
	sleep(1);
	printf("Thread %d : %d is global data\n",par, Global_data);
	
	*/
	sprintf(str[par],"xx/Service_%03d",par);

	ids[par] = dis_add_service(str[par],"C",str[par], strlen(str[par])+1, (void *)0, 0);
	dis_start_serving("xx");
	printf("Thread %d registered service %s\n",par,str[par]);
	dtq_start_timer(5,rout,par);
	/*
	mutex_unlock(&Global_mutex);
	*/
	/*
	while(1)
	  pause();
	  */
	/*
	return NULL;
	*/
}

