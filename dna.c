
/*
 * DNA (Delphi Network Access) implements the network layer for the DIM
 * (Delphi Information Managment) System.
 *
 * Started date   : 10-11-91
 * Written by     : C. Gaspar
 * UNIX adjustment: G.C. Ballintijn
 *
 * $Id: dna.c,v 1.1 1994/07/18 19:50:03 gerco Exp gerco $
 */

/* include files */

#define DNA
#include <dim.h>

/* global definitions */

#define READ_HEADER_SIZE	12
/*
#define TO_DBG		1 
*/

/* global variables */
typedef struct {
	char task_name[MAX_TASK_NAME];
	int port;
} PENDING_OPEN;

static PENDING_OPEN Pending_conns[MAX_CONNS];

static int DNA_Initialized = FALSE;


_PROTO( static void ast_read_h,     (int conn_id, int status, int size) );
_PROTO( static void ast_conn_h,     (int handle, int svr_conn_id,
                                     int protocol) );
_PROTO( static int dna_write_bytes, (int conn_id, void *buffer, int size,
									 int nowait) );
_PROTO( static void release_conn,   (int conn_id) );
_PROTO( static void save_node_task, (int conn_id, DNA_NET *buffer) );

/*
 * Routines common to Server and Client
 */

static int is_header( conn_id )
int conn_id;
{
	register DNA_CONNECTION *dna_connp = &Dna_conns[conn_id];
	register int ret;

	ret = 0;
	if( (vtohl(dna_connp->buffer[2]) == TRP_MAGIC) &&
	    (vtohl(dna_connp->buffer[1]) == 0) &&
	    (vtohl(dna_connp->buffer[0]) == READ_HEADER_SIZE) )
	{
		dna_connp->state = RD_HDR;
		ret = 1;
	} 
	else if( (vtohl(dna_connp->buffer[2]) == TST_MAGIC) &&
		   (vtohl(dna_connp->buffer[1]) == 0) &&
		   (vtohl(dna_connp->buffer[0]) == READ_HEADER_SIZE) )
	{
		dna_connp->state = RD_HDR;
		ret = 1;
	} 
	else if( (vtohl(dna_connp->buffer[2]) == HDR_MAGIC ) &&
		   (vtohl(dna_connp->buffer[0]) == READ_HEADER_SIZE ) )
	{
		dna_connp->state = RD_DATA;
		ret = 1;
	} 
	else 
	{
		print_date_time();
		printf( " conn: %d to %s@%s, expecting header\n", conn_id,
			Net_conns[conn_id].task, Net_conns[conn_id].node );
		printf( "buffer[0]=%d\n", vtohl(dna_connp->buffer[0]));
		printf( "buffer[1]=%d\n", vtohl(dna_connp->buffer[1]));
		printf( "buffer[2]=%x\n", vtohl(dna_connp->buffer[2]));
		printf( "closing the connection.\n" );
		dna_connp->read_ast(conn_id, NULL, 0, STA_DISC);
		ret = 0;
	}			
	return(ret);
}

static void read_data( conn_id)
register int conn_id;
{
	register DNA_CONNECTION *dna_connp = &Dna_conns[conn_id];

	if( !dna_connp->saw_init &&
	    vtohl(dna_connp->buffer[0]) == OPN_MAGIC)
	{
		save_node_task(conn_id, (DNA_NET *) dna_connp->buffer);
		dna_connp->saw_init = TRUE;
	} 
	else
	{
		dna_connp->read_ast(conn_id, dna_connp->buffer,
			dna_connp->full_size, STA_DATA);
	}
}

static void ast_read_h( conn_id, status, size )
register int conn_id;
int status;
register int size;
{
	register DNA_CONNECTION *dna_connp = &Dna_conns[conn_id];
	int tcpip_code;
	register int read_size, next_size;
	register char *buff;

	if(!dna_connp->buffer) /* The connection has already been closed */
		return;
	if(status == 1)
	{
	  /*
      		printf("conn %d \n",conn_id);
		*/
		next_size = dna_connp->curr_size;
		/*
		printf("conn %d : Read %d bytes, expecting %d\n",conn_id, size, next_size);
		*/
		buff = (char *) dna_connp->curr_buffer;
		if(size < next_size) 
		{
			read_size = ((next_size - size) > MAX_IO_DATA) ?
				MAX_IO_DATA : next_size - size;
			dna_connp->curr_size -= size;
			dna_connp->curr_buffer += size;
			tcpip_code = tcpip_start_read(conn_id, buff + size, 
				read_size, ast_read_h);
			if(tcpip_failure(tcpip_code)) 
			{
				dna_report_error(conn_id, tcpip_code,
					"ast_read_h / tcpip_start_read");
						return;
			}
			return;
		}
		/*
	{
		sigset_t oset;
		sigprocmask(0,0,&oset);
		printf("MASK = %x\n",oset.__sigbits[0]);
	}
	*/
		switch(dna_connp->state)
		{
			case RD_HDR :
			  /*
				printf("Read Header, %d bytes\n", size);
				*/
				if(is_header(conn_id))
				{
					if( dna_connp->state == RD_DATA )
					{
						next_size = vtohl(dna_connp->buffer[1]);
						/*
						printf("To read : %d bytes\n",next_size);
						*/
						dna_start_read(conn_id, next_size);
					}
					else
					{
						dna_connp->state = RD_HDR;
						dna_start_read(conn_id, READ_HEADER_SIZE);
					}
				}
				break;
			case RD_DATA :
			  /*
				printf("Read Data, %d bytes\n", size);
				*/
				read_data(conn_id);
				dna_connp->state = RD_HDR;
				dna_start_read(conn_id, READ_HEADER_SIZE);
				break;
		}
	} 
	else 
	{
		/* Connection lost. Signal upper layer ? */
		if(dna_connp->read_ast)
			dna_connp->read_ast(conn_id, NULL, 0, STA_DISC);
/*		release_conn(conn_id); */
	}
}


dna_start_read(conn_id, size)
register int conn_id, size;
{
	register DNA_CONNECTION *dna_connp = &Dna_conns[conn_id];
	register int tcpip_code, read_size;
	
	if(!dna_connp->busy)
		return(0);

	dna_connp->curr_size = size;
	dna_connp->full_size = size;
	if(size > dna_connp->buffer_size) {
		dna_connp->buffer =
				(int *) realloc(dna_connp->buffer, size);
		dna_connp->buffer_size = size;
	}
	dna_connp->curr_buffer = (char *) dna_connp->buffer;
	read_size = (size > MAX_IO_DATA) ? MAX_IO_DATA : size ;

	tcpip_code = tcpip_start_read(conn_id, dna_connp->curr_buffer,
				  read_size, ast_read_h);
	if(tcpip_failure(tcpip_code)) {
		dna_report_error(conn_id, tcpip_code,
			"dna_start_read / tcpip_start_read");

		return(0);
	}

	return(1);
}								


static int dna_write_bytes( conn_id, buffer, size, nowait )
register int conn_id, size, nowait;
void *buffer;
{
	register DNA_CONNECTION *dna_connp = &Dna_conns[conn_id];
	register int size_left, tcpip_code, wrote;
	register char *p;
	int retries = 3000;
	float wait_time = 0.01;

	p = (char *) buffer;
	size_left = size;
	do {
		size = (size_left > MAX_IO_DATA) ? MAX_IO_DATA : size_left ;
#ifdef VMS
		if(nowait)
		{
			while(--retries)
			{
				if(wrote = tcpip_write_nowait(conn_id, p, size))
					break;
				if(!tcpip_would_block(wrote))
					return(0);
				if(retries == 2990)
					dna_report_error(conn_id, tcpip_code,
						"dna_write / retrying...");
				lib$wait(&wait_time);
			}
			if(!retries)
				return(0);
		}
		else
			wrote = tcpip_write(conn_id, p, size);
#else
		wrote = tcpip_write(conn_id, p, size);
#endif
		if( tcpip_failure(wrote) )
			return(0);
		p += wrote;
		size_left -= wrote;
	} while(size_left > 0);

	if(retries < 2990)
	{
		print_date_time();
		printf("Recovered after %d retries\n",3000 - retries);
	}
	return(1);
}

void dna_test_write(conn_id)
register int conn_id;
{
	register DNA_CONNECTION *dna_connp = &Dna_conns[conn_id];
	register int tcpip_code, size;
	DNA_HEADER test_pkt;
	register DNA_HEADER *test_p = &test_pkt;

	if(!dna_connp->busy)
	{
		return;
    }
	if(dna_connp->writing)
	{
		return;
    }
	test_p->header_size = htovl(READ_HEADER_SIZE);
	test_p->data_size = 0;
	test_p->header_magic = htovl(TST_MAGIC);
	tcpip_code = dna_write_bytes(conn_id, &test_pkt, READ_HEADER_SIZE,0);
	if(tcpip_failure(tcpip_code)) {
		/* Connection lost. Signal upper layer ? */
		if(dna_connp->read_ast)
			dna_connp->read_ast(conn_id, NULL, 0, STA_DISC);
/*		release_conn(conn_id); */
		return;
	}
}

dna_write(conn_id, buffer, size)
register int conn_id, size;
void *buffer;
{
	register DNA_CONNECTION *dna_connp = &Dna_conns[conn_id];
	DNA_HEADER header_pkt;
	register DNA_HEADER *header_p = &header_pkt;
	int tcpip_code;
	DISABLE_AST
	if(!dna_connp->busy)
	{
		ENABLE_AST
		return(2);
    }
	dna_connp->writing = TRUE;
	header_p->header_size = htovl(READ_HEADER_SIZE);
	header_p->data_size = htovl(size);
	header_p->header_magic = htovl(HDR_MAGIC);
	tcpip_code = dna_write_bytes(conn_id, &header_pkt, READ_HEADER_SIZE,0);
	if(tcpip_failure(tcpip_code)) {
		dna_report_error(conn_id, tcpip_code,
			"dna_write / tcpip_write");

		dna_connp->writing = FALSE;
		ENABLE_AST
		return(0);
	}

	tcpip_code = dna_write_bytes(conn_id, buffer, size,0);
	if(tcpip_failure(tcpip_code)) {
		dna_report_error(conn_id, tcpip_code,
			"dna_write / tcpip_write");

		dna_connp->writing = FALSE;
		ENABLE_AST
		return(0);
	}

	dna_connp->writing = FALSE;
	ENABLE_AST
	return(1);
}	

dna_write_nowait(conn_id, buffer, size)
register int conn_id, size;
void *buffer;
{
	register DNA_CONNECTION *dna_connp = &Dna_conns[conn_id];
	DNA_HEADER header_pkt;
	register DNA_HEADER *header_p = &header_pkt;
	int tcpip_code;
	DISABLE_AST
	if(!dna_connp->busy)
	{
		ENABLE_AST
		return(2);
    }
	dna_connp->writing = TRUE;
	header_p->header_size = htovl(READ_HEADER_SIZE);
	header_p->data_size = htovl(size);
	header_p->header_magic = htovl(HDR_MAGIC);
	tcpip_code = dna_write_bytes(conn_id, &header_pkt, READ_HEADER_SIZE, 1);
	if(tcpip_failure(tcpip_code)) {
		dna_report_error(conn_id, tcpip_code,
			"dna_write / tcpip_write");

		dna_connp->writing = FALSE;
		ENABLE_AST
		return(0);
	}

	tcpip_code = dna_write_bytes(conn_id, buffer, size, 1);
	if(tcpip_failure(tcpip_code)) {
		dna_report_error(conn_id, tcpip_code,
			"dna_write / tcpip_write");

		dna_connp->writing = FALSE;
		ENABLE_AST
		return(0);
	}

	dna_connp->writing = FALSE;
	ENABLE_AST
	return(1);
}	

/* Server Routines */

static void ast_conn_h(handle, svr_conn_id, protocol)
int handle;
register int svr_conn_id;
int protocol;
{
	register DNA_CONNECTION *dna_connp;
	register int tcpip_code;
	register int conn_id;

	conn_id = conn_get();
	if(!conn_id)
		dim_panic("In ast_conn_h: No more connections\n");
	tcpip_code = tcpip_open_connection( conn_id, handle );
	if(tcpip_failure(tcpip_code))
	{
		dna_report_error(conn_id, tcpip_code,
			"ast_conn_h / tcpip_open_connection");
		conn_free(conn_id);
	} else {
		dna_connp = &Dna_conns[conn_id] ;
		dna_connp->state = RD_HDR;
		dna_connp->buffer = (int *)malloc(TCP_RCV_BUF_SIZE);
		if(!dna_connp->buffer)
			printf("Error in DNA - handle_connection malloc returned 0\n");
		dna_connp->buffer_size = TCP_RCV_BUF_SIZE;
		dna_connp->read_ast = Dna_conns[svr_conn_id].read_ast;
		dna_connp->saw_init = FALSE;
		dna_start_read(conn_id, READ_HEADER_SIZE); /* sizeof(DNA_NET) */
		/* Connection arrived. Signal upper layer ? */
		dna_connp->read_ast(conn_id, NULL, 0, STA_CONN);
	}
	tcpip_code = tcpip_start_listen(svr_conn_id, ast_conn_h);
	if(tcpip_failure(tcpip_code))
	{
		dna_report_error(svr_conn_id, tcpip_code,
			"ast_conn_h / tcpip_start_listen");
	}
}


int dna_open_server(task, read_ast, protocol, port)
char *task;
void (*read_ast)();
int *protocol, *port;
{
	register DNA_CONNECTION *dna_connp;
	register int tcpip_code, err_code;
	register int conn_id;

	if(!DNA_Initialized)
	{
		conn_arr_create(SRC_DNA);
		DNA_Initialized = TRUE;
	}
	*protocol = PROTOCOL;
/*
	if( PROTOCOL == BOTH ) {
		conn_id = conn_get();
		if(!conn_id)
			dim_panic("In dna_open_server: No more connections\n");
		tcpip_code = tcpip_open_server(conn_id, task, TCPIP, port);
		if(tcpip_failure(tcpip_code))
		{
			dna_report_error(conn_id, tcpip_code,
				"dna_open_server / tcpip_open_server");
			conn_free(conn_id);
		}
		else
		{
			dna_connp = &Dna_conns[conn_id];
			dna_connp->read_ast = read_ast;
			dna_connp->writing = FALSE;
			tcpip_code = tcpip_start_listen(conn_id, ast_conn_h);
			if(tcpip_failure(tcpip_code))
			{
				dna_report_error(conn_id, tcpip_code,
					"dna_open_server / tcpip_start_listen");
				return(0);
			}
		}
		conn_id = conn_get();
		if(!conn_id)
			dim_panic("In dna_open_server: No more connections\n");
		tcpip_code = tcpip_open_server(conn_id, task, DECNET, port);
		if(tcpip_failure(tcpip_code))
		{
			dna_report_error(conn_id, tcpip_code,
				"dna_open_server / tcpip_open_server");
			conn_free(conn_id);
			return(0);
		}
		dna_connp = &Dna_conns[conn_id];
		dna_connp->writing = FALSE;
		dna_connp->read_ast = read_ast;
		tcpip_code = tcpip_start_listen(conn_id, ast_conn_h);
		if(tcpip_failure(tcpip_code))
		{
			dna_report_error(conn_id, tcpip_code,
				"dna_open_server / tcpip_start_listen");
			return(0);
		}
	}
	else
	{
*/
		conn_id = conn_get();
		if(!conn_id)
			dim_panic("In dna_open_server: No more connections\n");
		Dna_conns[conn_id].protocol = TCPIP;
		tcpip_code = tcpip_open_server(conn_id, task, port);
		if(tcpip_failure(tcpip_code))
		{
			dna_report_error(conn_id, tcpip_code,
				"dna_open_server / tcpip_open_server");
			conn_free(conn_id);
			return(0);
		}
		dna_connp = &Dna_conns[conn_id];
		dna_connp->writing = FALSE;
		dna_connp->read_ast = read_ast;
		tcpip_code = tcpip_start_listen(conn_id, ast_conn_h);
		if(tcpip_failure(tcpip_code))
		{
			dna_report_error(conn_id, tcpip_code,
				"dna_open_server / tcpip_start_listen");
			return(0);
		}
/*
	}
*/
	return(conn_id);
}


dna_get_node_task(conn_id, node, task)
int conn_id;
char *node;
char *task;
{
	if(Dna_conns[conn_id].busy)
		tcpip_get_node_task(conn_id, node, task);
	else
		node[0] = '\0';
}


/* Client Routines */

dna_set_test_write(conn_id, time)
int conn_id, time;
{
	tcpip_set_test_write(conn_id, time);
}

static int ins_pend_conn( task, port )
char *task;
int port;                                      
{
	register PENDING_OPEN *pending_connp;
	register int i;

	for( i = 1, pending_connp = &Pending_conns[1]; i < MAX_CONNS; 
		i++, pending_connp++ )
	{
		if( pending_connp->task_name[0] == '\0' )
		{
			strcpy(pending_connp->task_name, task);
			pending_connp->port = port;
			return(i);
		}
	}
	return(0);
}

static int find_pend_conn( task, port )
register char *task;
register int port;
{
	register PENDING_OPEN *pending_connp;
	register int i;

	for( i = 1, pending_connp = &Pending_conns[1]; i < MAX_CONNS; 
		i++, pending_connp++ )
	{
		if( (!strcmp(pending_connp->task_name, task)) &&
			(pending_connp->port == port) )
		{
			return(i);
		}
	}
	return(0);
}


static rel_pend_conn( conn_id )
int conn_id;
{
	Pending_conns[conn_id].task_name[0] = '\0';
}	


int dna_open_client(server_node, server_task, port, server_protocol, read_ast)
char *server_node, *server_task;
int port;
int server_protocol;
void (*read_ast)();
{
	register DNA_CONNECTION *dna_connp;
	char str[256];
	register int tcpip_code, conn_id, id;
	DNA_NET local_buffer;

	if(!DNA_Initialized) {
		conn_arr_create(SRC_DNA);
		DNA_Initialized = TRUE;
	}
	if( !(conn_id = conn_get()) )
		dim_panic("In dna_open_client: No more connections\n");
	Dna_conns[conn_id].protocol = TCPIP;
	tcpip_code = tcpip_open_client(conn_id, server_node, server_task, port);
	if( tcpip_failure(tcpip_code) )
	{
#ifdef VMS
		if(!strstr(server_node,"fidel"))
		{
#endif
		if(!find_pend_conn(server_task, port))
		{
			sprintf( str,"dna_open_client / tcpip_open_client : %s on %s ", 
				server_task, server_node );
			dna_report_error( conn_id, tcpip_code, str );
			ins_pend_conn(server_task, port);
		}
#ifdef VMS
		}
#endif
		tcpip_close(conn_id);
		conn_free( conn_id );
		return(0);
	}
	if(id = find_pend_conn(server_task, port))
	{
		sprintf( str,"dna_open_client / tcpip_open_client : %s on %s\n\ttcpip: Connection %d established", 
			server_task, server_node, conn_id );
		dna_report_error( 0, -1, str );
		rel_pend_conn(id);
	}
	dna_connp = &Dna_conns[conn_id] ;
	dna_connp->state = RD_HDR;
	dna_connp->writing = FALSE;
	dna_connp->buffer = (int *)malloc(TCP_RCV_BUF_SIZE);
	if(!dna_connp->buffer)
		printf("Error in DNA - open_client malloc returned 0\n");
	dna_connp->buffer_size = TCP_RCV_BUF_SIZE;
	dna_connp->read_ast = read_ast;
	dna_connp->saw_init = TRUE;	/* we send it! */
	dna_start_read(conn_id, READ_HEADER_SIZE);
	local_buffer.code = htovl(OPN_MAGIC);
	get_node_name(local_buffer.node);
	get_proc_name(local_buffer.task);
	tcpip_code = dna_write(conn_id, &local_buffer, sizeof(local_buffer));
	if (tcpip_failure(tcpip_code))
	{
/*
		dna_report_error(conn_id, tcpip_code,
			"dna_open_client / tcpip_write");
*/
		return(0);
	};
	read_ast(conn_id, NULL, 0, STA_CONN);
	return(conn_id);
}
	
dna_close(conn_id)
int conn_id;
{
	release_conn(conn_id);
}

/* connection managment routines */

static void release_conn(conn_id)
register int conn_id;
{
	register DNA_CONNECTION *dna_connp = &Dna_conns[conn_id] ;

	if(dna_connp->busy)
	{ 
		tcpip_close(conn_id);
		if(dna_connp->buffer)
		{
			free(dna_connp->buffer);
			dna_connp->buffer = 0;
			dna_connp->buffer_size = 0;
		}
		dna_connp->read_ast = NULL;
		conn_free(conn_id);
	}
}

void dna_report_error(conn_id, code, routine_name)
int conn_id, code;
char *routine_name;
{
	print_date_time();
	printf(" Executing %s\n", routine_name);
	if(conn_id)
	{
		if(Net_conns[conn_id].node[0])
			printf("\tConn %d, %s on node %s :\n", conn_id,
		       Net_conns[conn_id].task, Net_conns[conn_id].node);
		else
			printf("\tConn %d :\n", conn_id);
	}
	if(code != -1)
	{
		printf("\t");
		tcpip_report_error(code);
	}
}


static void save_node_task(conn_id, buffer)
int conn_id;
DNA_NET *buffer;
{
	strcpy(Net_conns[conn_id].node, buffer->node);
	strcpy(Net_conns[conn_id].task, buffer->task);
}

