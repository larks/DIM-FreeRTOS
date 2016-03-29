#include <stdio.h>
#include <thread.h>
#include <dis.h>

#define NUM_THREADS 10

void *change_global_data(void *);     /*  for thr_create()   */


main(int argc,char * argv[])  
{
	int i=0;
	
       	dim_init_threads();
	for (i=0; i< NUM_THREADS; i++)     {
	  thr_create(NULL, 0, change_global_data, (void *)i, 0, NULL);
	}
	
	while(thr_join(NULL, NULL, NULL));
	printf("Out of thread join\n");
	while(1)
	  pause();
}

char str[10][80];
int ids[10];

rout(tag, str, size)
int tag; 
{
  printf("Thread %d updating...\n",tag);
  dis_update_service(ids[tag]);
  dtq_start_timer(5,rout,tag);
}


void *change_global_data(void *p)
{
  int par;
    
	static mutex_t Global_mutex;
	
	char serv_name[80];

	par = (int)p;
	/*
	mutex_lock(&Global_mutex);
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
	
	return NULL;
	
}

