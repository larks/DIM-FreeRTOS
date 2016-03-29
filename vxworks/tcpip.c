/*
 * DNA (Delphi Network Access) implements the network layer for the DIM
 * (Delphi Information Managment) System.
 *
 * Started           : 10-11-91
 * Last modification : 29-07-94
 * Written by        : C. Gaspar
 * Adjusted by       : G.C. Ballintijn
 *
 * $Id: tcpip.c,v 1.1 1994/07/18 19:50:03 gerco Exp gerco $
 */

#ifdef solaris
#define BSD_COMP
#include <thread.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <errno.h>
#ifndef VxWorks
#include <netdb.h>
#else /* VxWorks */
#include <ioLib.h>
#include <sockLib.h>
#include <hostLib.h>
#endif
/*
#include <stdarg.h>
*/
#include <stdio.h>
#include "dim.h"

/*
#define LISTDBG
#define SIGDBG
*/

static int Tcpip_init_done = FALSE;		/* Is this module initialized? */
static int	queue_id;
/*
void (*FD_rout)();
void *FD_par; 
*/

#define VX_TASK_PRIO 150
#define VX_TASK_SSIZE 20000

int tcpip_init()
{
  int tid;
  void io_handler_task();

  if(tid = taskSpawn("DIM_IO_svr", VX_TASK_PRIO, 0, VX_TASK_SSIZE, 
		     io_handler_task, 0) == ERROR)
    {
      perror("Task Spawn");
    }
   Tcpip_init_done = 1;

  return(tid);
}

static int enable_sig(conn_id)
int conn_id;
{
  static int pfd = 0,ret;

      if(pfd <= 0)
	{
	  taskDelay(50);
	  pfd = open("/pipe/DIM_IO_pp",O_WRONLY,0);
	}
      if(pfd > 0)
	ret = write(pfd, "QUIT", 5);
      return(1);
}


static void dump_list()
{
	int	i;

	for( i = 1; i < Curr_N_Conns; i++ )
		if( Dna_conns[i].busy ) {
			printf( "dump_list: conn_id=%d reading=%d\n",
				i, Net_conns[i].reading );
		}
}


static void list_to_fds( fds )
fd_set *fds;
{
	int	i;

	FD_ZERO( fds ) ;
	for( i = 1; i < Curr_N_Conns; i++ )
		if( Dna_conns[i].busy )
		  {
			FD_SET( Net_conns[i].channel, fds );
		  }
}


static int fds_get_entry( fds, conn_id ) 
fd_set *fds;
int *conn_id;
{
	int	i;

	for( i = 1; i < Curr_N_Conns; i++ )
		if( Dna_conns[i].busy &&
		    FD_ISSET(Net_conns[i].channel, fds) )
		{
			*conn_id = i;
			return 1;
		}
	
	return 0;
}

/*
static void tcpip_test_write( tag )
int tag;
{
	int conn_id;
	time_t cur_time;
	int s, count;

	cur_time = time(NULL);
	for( conn_id = 1; conn_id < Curr_N_Conns; conn_id++ )
		if( Dna_conns[conn_id].busy &&
		    Dna_conns[conn_id].protocol == TCPIP &&
		    Net_conns[conn_id].reading &&
		    !Dna_conns[conn_id].writing )
		{	
			if( ioctl( Net_conns[conn_id].channel, FIONREAD,
				   (int)&count ) == 0 && count > 0 )
				return;
			if( cur_time - Net_conns[conn_id].last_used >
					TEST_TIME )
				dna_test_write( conn_id );
		}
}
*/

static void tcpip_test_write( conn_id )
int conn_id;
{
	/* Write to every socket we use, which uses the TCPIP protocol,
	 * which has an established connection (reading), which is currently
	 * not writing data, so we can check if it is still alive.
	 */
	time_t cur_time;
	
	cur_time = time(NULL);
	if( cur_time - Net_conns[conn_id].last_used > Net_conns[conn_id].timeout )
	{
/*
#ifndef VxWorks
		dna_test_write( conn_id );
#endif
*/
	}
}

tcpip_set_test_write(conn_id, timeout)
int conn_id, timeout;
{
	Net_conns[conn_id].timr_ent = dtq_add_entry( queue_id, timeout, 
		tcpip_test_write, conn_id );
	Net_conns[conn_id].timeout = timeout;
	Net_conns[conn_id].last_used = time(NULL);
}

static void pipe_sig_handler( num )
int num;
{
/*
	printf( "*** pipe_sig_handler called ***\n" );
*/
}


static void do_read( conn_id )
int conn_id;
{
	/* There is 'data' pending, read it.
	 */
	int	len, ret, totlen, size, count;
	char	*p;

	if( ioctl( Net_conns[conn_id].channel, FIONREAD, (int)&count ) == 0 &&
	    count == 0 )
	{	/* Connection closed by other side. */
		Net_conns[conn_id].read_rout( conn_id, -1, 0 );
		return;
	}

	size = Net_conns[conn_id].size;
	p = Net_conns[conn_id].buffer;
	totlen = 0;
	while( size > 0 &&
	       (ret = ioctl( Net_conns[conn_id].channel, FIONREAD,
			     (int)&count ) == 0) &&
	       count > 0 )
	{
		if( (len = read(Net_conns[conn_id].channel, p, size)) <= 0 ) {
			if( len == -1 )
				perror( "read" );
			else
				printf( "Data available, but got an EOF!\n" );
			Net_conns[conn_id].read_rout( conn_id, -1, 0 );
			return;
		} else {
		  /*
		  printf("tcpip: read %d bytes:\n",len); 
		printf( "buffer[0]=%d\n", vtohl((int *)p[0]));
		printf( "buffer[1]=%d\n", vtohl((int *)p[1]));
		printf( "buffer[2]=%x\n", vtohl((int *)p[2]));
		*/
			totlen += len;
			size -= len;
			p += len;
		}
	}

	Net_conns[conn_id].last_used = time(NULL);
	if(ret == -1 ) 
		perror( "ioctl" );
	else
	  {
		Net_conns[conn_id].read_rout( conn_id, 1, totlen );/* 1==OK */
	  }
}


do_accept( conn_id )
int conn_id;
{
	/* There is a 'connect' pending, serve it.
	 */
	struct sockaddr_in	other;
	int			othersize;

	othersize = sizeof(other);
	memset( (char *) &other, 0, othersize );
	Net_conns[conn_id].mbx_channel = accept( Net_conns[conn_id].channel,
						 (struct sockaddr*)&other, &othersize );
	if( Net_conns[conn_id].mbx_channel < 0 ) {
		perror( "accept" );
		return;
	}

	Net_conns[conn_id].last_used = time(NULL);
	Net_conns[conn_id].read_rout( Net_conns[conn_id].mbx_channel,
				      conn_id, TCPIP );
}

void io_handler_task( main_conn_id )
int main_conn_id;
{
	/* wait for an IO signal, find out what is happening and
	 * call the right routine to handle the situation.
	 */
	fd_set	rfds;
	int	conn_id, ret, rcount, count;
	int pfd;
	char pipe_buf[10];

	pfd = pipeDevCreate("/pipe/DIM_IO_pp", 1000, 5);
	pfd = open("/pipe/DIM_IO_pp",O_RDONLY,0);

	while(1)
	{
	    list_to_fds( &rfds );
	    FD_SET( pfd, &rfds );
	    ret = select(FD_SETSIZE, &rfds, NULL, NULL, NULL);
	    if(ret < 1) {
	      /*
		if( ret < 0 )
		  perror( "select" );
		  */
	      continue;
	    }
	    if(FD_ISSET(pfd, &rfds) )
	    {
	      read(pfd, pipe_buf, 10);
	      FD_CLR( pfd, &rfds );
	    }
	    while( ret = fds_get_entry( &rfds, &conn_id ) > 0 ) {
		  /*
		printf("fds_get_entry returned %d\n",ret);
		*/
/*
		disable_sig( conn_id );
*/
		if( Net_conns[conn_id].reading )
		{
			do
			{
				do_read( conn_id );
				ioctl( Net_conns[conn_id].channel, FIONREAD, 
				       (int)&count );
/*
				printf("Read %d, But there was still %d more to read...\n",
					totlen,count);
*/
			}while(count > 0 );
		}
		else
		{
			do_accept( conn_id );
		}
		FD_CLR( Net_conns[conn_id].channel, &rfds );
	    }
         }
}

void dim_io(fd)
int fd;
{
	/* We got an IO signal, lets find out what is happening and
	 * call the right routine to handle the situation.
	 */
	int	i, conn_id = 0, ret, rcount, count;

	DISABLE_AST

	for( i = 1; i < Curr_N_Conns; i++ )
		if( Dna_conns[i].busy && (Net_conns[i].channel == fd) )
		{
			conn_id = i;
			break;
		}
	
	if(conn_id)
	{
		if( Net_conns[conn_id].reading )
		{
			do
			{
				do_read( conn_id );
				ioctl( Net_conns[conn_id].channel, FIONREAD, (int)&count );
/*
				printf("Read %d, But there was still %d more to read...\n",
					totlen,count);
*/
			}while(count > 0 );
		}
		else
		{
			do_accept( conn_id );
		}
	}
	ENABLE_AST
}


	/* Do the signal calls, and get an repeating timer
	 * interrupt.
	 */

int tcpip_start_read( conn_id, buffer, size, ast_routine )
int size, conn_id;
char *buffer;
void (*ast_routine)();
{
	int pid, ret = 1, flags = 1;
	/* Install signal handler stuff on the socket, and record
	 * some necessary information: we are reading, and want size
	 * as size, and use buffer.
	 */

	Net_conns[conn_id].read_rout = ast_routine;
	Net_conns[conn_id].buffer = buffer;
	Net_conns[conn_id].size = size;
	Net_conns[conn_id].reading = TRUE;
	if(enable_sig( conn_id ) == -1)
	{
	    printf("START_READ - enable_sig returned -1\n");
	    return(0);
	}	
	return(1);
}


int tcpip_open_client( conn_id, node, task, port )
int conn_id;
char *node, *task;
int port;
{
	/* Create connection: create and initialize socket stuff. Try
	 * and make a connection with the server.
	 */
	struct sockaddr_in sockname;
#ifndef VxWorks
	struct hostent *host;
#else
	int host_addr;
	char tmp_node[MAX_NAME];
#endif
	int path, par, val, ret_code;

	/*
	printf("Open conn to %s@%s, port %d, conn_id = %d\n",task, node,
	       port, conn_id);
	       */
	if(!Tcpip_init_done)
	  tcpip_init();
	strcpy(tmp_node,node);
	*(strchr(tmp_node,'.')) = '\0';
       	host_addr = hostGetByName(tmp_node);

	if( (path = socket(AF_INET, SOCK_STREAM, 0)) == -1 ) 
	{
	  perror("socket");
		return(0);
	}

	val = 1;
      
	if ((ret_code = setsockopt(path, IPPROTO_TCP, TCP_NODELAY, 
			(char*)&val, sizeof(val))) == -1 ) 
	{
		printf("Couln't set TCP_NODELAY\n");
		close(path); 
		return(0);
	}

	sockname.sin_family = PF_INET;
#ifndef VxWorks /* XXX: Not sure... */
	sockname.sin_addr = *((struct in_addr *) host->h_addr);
#else
	sockname.sin_addr = *((struct in_addr *) &host_addr);
#endif
	sockname.sin_port = htons((ushort) port); /* port number to send to */
	while(connect(path, (struct sockaddr*)&sockname, sizeof(sockname)) == -1 )
	{
		if(errno != EINTR)
		{
			close(path);
			return(0);
		}
	}
	strcpy( Net_conns[conn_id].node, node );
	strcpy( Net_conns[conn_id].task, task );
	Net_conns[conn_id].channel = path;
	Net_conns[conn_id].port = port;
	Net_conns[conn_id].last_used = time(NULL);
	/*
	if(FD_rout)
		FD_rout(FD_par, Net_conns[conn_id].channel, 1);
		*/
	/*
	printf("tcpip_open_client %s@%s conn %d\n",task, node, conn_id);
	*/
	return(1);
}


int tcpip_open_server( conn_id, task, port )
int conn_id, *port;
char *task;
{
	/* Create connection: create and initialize socket stuff,
	 * find a free port on this node.
	 */
	struct sockaddr_in sockname;
	int path, par, val, ret_code;

	if(!Tcpip_init_done)
	  tcpip_init();
	if( (path = socket(AF_INET, SOCK_STREAM, 0)) == -1 ) 
		return(0);


	val = 1;
	if ((ret_code = setsockopt(path, IPPROTO_TCP, TCP_NODELAY, 
			(char*)&val, sizeof(val))) == -1 ) 

	{
		printf("Couln't set TCP_NODELAY\n");
		close(path); 
		return(0);
	}
	
	if( *port == SEEK_PORT ) {	/* Search a free one. */
		*port = START_PORT_RANGE - 1;
		do {
			(*port)++;
			sockname.sin_family = AF_INET;
			sockname.sin_addr.s_addr = INADDR_ANY;
			sockname.sin_port = htons((ushort) *port);
			if( *port > STOP_PORT_RANGE ) {
				errno = EADDRNOTAVAIL;	/* fake an error code */
				close(path);
				return(0);
			}
		} while( bind(path, (struct sockaddr*)&sockname, sizeof(sockname)) == -1 );
	} else {
		val = 1;
		if( setsockopt(path, SOL_SOCKET, SO_REUSEADDR, (char*)&val, 
			sizeof(val)) == -1 )
		{
			printf("Couln't set SO_REUSEADDR\n");
			close(path); 
			return(0);
		}
		sockname.sin_family = AF_INET;
		sockname.sin_addr.s_addr = INADDR_ANY;
		sockname.sin_port = htons((ushort) *port);
		if( bind(path, (struct sockaddr*) &sockname, sizeof(sockname)) == -1 )
		{
			close(path);
			return(0);
		}
	}

	if( listen(path, 5) == -1 )
	{
		close(path);
		return(0);
	}

	strcpy( Net_conns[conn_id].node, "MYNODE" );
	strcpy( Net_conns[conn_id].task, task );
	Net_conns[conn_id].channel = path;
	Net_conns[conn_id].port = *port;
	Net_conns[conn_id].last_used = time(NULL);

	/*
	if(FD_rout)
		FD_rout(FD_par, Net_conns[conn_id].channel, 1);
		*/
/*	
	printf("tcpip_open_server conn %d, port = %d\n",conn_id, *port);
*/
	return(1);
}


int tcpip_start_listen( conn_id, ast_routine )
int conn_id;
void (*ast_routine)();
{
	int pid, ret=1, flags = 1;
	/* Install signal handler stuff on the socket, and record
	 * some necessary information: we are NOT reading, thus
	 * no size.
	 */

	Net_conns[conn_id].read_rout = ast_routine;
	Net_conns[conn_id].size = -1;
	Net_conns[conn_id].reading = FALSE;
	if(enable_sig( conn_id ) == -1)
	{
	    printf("START_LISTEN - enable_sig returned -1\n");
		return(0);
	}

/*
	printf("tcpip_start_listen conn %d\n",conn_id);
*/
	return(1);
}


int tcpip_open_connection( conn_id, path )
int conn_id, path;
{
	/* Fill in/clear some fields, the node and task field
	 * get filled in later by a special packet.
	 */
	int par, val, ret_code;


	if(!Tcpip_init_done)
	  tcpip_init();
	val = 1;
	if ((ret_code = setsockopt(path, IPPROTO_TCP, TCP_NODELAY, 
			(char*)&val, sizeof(val))) == -1 ) 
	{
		printf("Couln't set TCP_NODELAY\n");
		close(path); 
		return(0);
	}
	Net_conns[conn_id].channel = path;
	Net_conns[conn_id].node[0] = 0;
	Net_conns[conn_id].task[0] = 0;
	Net_conns[conn_id].port = 0;

	/*
	if(FD_rout)
		FD_rout(FD_par, Net_conns[conn_id].channel, 1);
		*/
/*
	printf("tcpip_open_connection conn %d\n",conn_id);
*/
	return(1);
}


void tcpip_get_node_task( conn_id, node, task )
int conn_id;
char *node, *task;
{
	strcpy( node, Net_conns[conn_id].node );
	strcpy( task, Net_conns[conn_id].task );
}


int tcpip_write( conn_id, buffer, size )
int conn_id, size;
char *buffer;
{
	/* Do a (synchronous) write to conn_id.
	 */
	int	wrote;

	wrote = write( Net_conns[conn_id].channel, buffer, size );
	if( wrote == -1 ) {
		perror( "write" );
		Net_conns[conn_id].read_rout( conn_id, -1, 0 );
		return(0);
	}

	return(wrote);
}

int tcpip_write_nowait( conn_id, buffer, size )
int conn_id, size;
char *buffer;
{
	/* Do a (synchronous) write to conn_id.
	 */
	int	wrote;

	wrote = write( Net_conns[conn_id].channel, buffer, size );
	if( wrote == -1 ) {
		perror( "write" );
		Net_conns[conn_id].read_rout( conn_id, -1, 0 );
		return(0);
	}

	return(wrote);
}


int tcpip_close( conn_id )
int conn_id;
{
	/* Clear all traces of the connection conn_id.
	 */
  /*
	if(FD_rout)
		FD_rout(FD_par, Net_conns[conn_id].channel, 0);
		*/
	close( Net_conns[conn_id].channel );
	Net_conns[conn_id].channel = 0;
	Net_conns[conn_id].port = 0;
	Net_conns[conn_id].node[0] = 0;
	Net_conns[conn_id].task[0] = 0;

/*
	printf("tcpip_close conn %d\n",conn_id);
*/
#ifdef VxWorks
	enable_sig(conn_id);
#endif
	return(1);
}


int tcpip_failure( code )
int code;
{
	return(!code);
}


void tcpip_report_error( code )
int code;
{
	perror( "tcpip" );
}
