/*
#define _REENTRANT
*/
#include <stdio.h>
/*
#include <thread.h>
*/
#include <dim.h>
#include <dic.h>

#define NUM_THREADS 5

void *change_global_data();     /*  for thr_create()   */

rout(tag, str, size)
int *tag; 
char *str;
int *size;
{
  printf("Thread %d received %s\n",*tag, str);
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
	sprintf(serv_name,"xx/Service_%03d",i);
	{
	  /*
	DISABLE_AST
	*/
	dic_info_service(serv_name,MONITORED,0,0,0,rout,i,"no_link",8);
	/*
	ENABLE_AST
	*/
	  }
	printf("Thread %d requested service %s\n",i,serv_name);
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
	sprintf(serv_name,"xx/Service_%03d",par);
	dic_info_service(serv_name,MONITORED,0,0,0,rout,par,"no_link",8);
	printf("Thread %d requested service %s\n",par,serv_name);
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

