/*
 * DNA (Delphi Network Access) implements the network layer for the DIM
 * (Delphi Information Managment) System.
 *
 * Started date   : 10-11-91
 * Written by     : C. Gaspar
 * UNIX adjustment: G.C. Ballintijn
 *
 * $Id: utilities.c,v 1.1 1994/07/18 19:50:03 gerco Exp gerco $
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <dim.h>
#ifndef VxWorks
#include <netdb.h>
#else
#include <hostLib.h>
#endif


int get_proc_name(proc_name)
char *proc_name;
{
#ifndef VxWorks
	sprintf( proc_name, "%d", getpid() );
#else
	sprintf( proc_name, "%d", taskIdSelf() );      
#endif
	return(1);
}


int get_node_name(node_name)
char *node_name;
{
#ifndef VxWorks
struct hostent *host;
#endif
char	*p;

	gethostname(node_name, MAX_NODE_NAME);
#ifndef VxWorks
	if(!strchr(node_name,'.'))
	{
		if ((host = gethostbyname(node_name)) == (struct hostent *)0) 
			return(0);
		strcpy(node_name,host->h_name);
	}
#endif
	if(!strchr(node_name,'.'))
	{
		if( (p = getenv("DIM_HOST_NODE")) == NULL )
			return(0);
		else 
		{
			strcpy( node_name, p );
			return(1);
		}
	}
}


void print_date_time()
{
	time_t t;

	t = time((time_t *)0);
	printf( ctime(&t) );
}


void dim_panic( s )
char *s;
{
	printf( "\n\nDNA library panic: %s\n\n", s );
	exit(0);
}

int get_dns_node_name( node_name )
char *node_name;
{
	char	*p;

	if( (p = getenv("DIM_DNS_NODE")) == NULL )
		return(0);
	else {
		strcpy( node_name, p );
		return(1);
	}
}







