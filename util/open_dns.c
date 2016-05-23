/*
 * A utility file. 
 *
 * Started date   : 10-11-91
 * Written by     : C. Gaspar
 * UNIX adjustment: G.C. Ballintijn
 *
 * $Id$
 */

#include <dim.h>


typedef struct {
	char node_name[MAX_NODE_NAME];
	char task_name[MAX_TASK_NAME];
	void (*recv_rout)();
	TIMR_ENT *timr_ent;
} PENDING_CONN;

static PENDING_CONN Pending_conns[MAX_CONNS];
static int Timer_q = 0;


_PROTO( void retry_dns_connection,    ( int conn_pend_id ) );
static int get_free_pend_conn(), find_pend_conn(), rel_pend_conn();

#ifdef OSK
#	define bigbuf static
#else
#	define bigbuf
#endif

int open_dns( recv_rout, tmout_min, tmout_max )
void (*recv_rout)();
int tmout_min, tmout_max;
{
	bigbuf char nodes[255], all_nodes[255];
	bigbuf char str[132];
	register char *dns_node, *ptr; 
	register int conn_id, conn_pend_id;
	register PENDING_CONN *conn_pend;
	register int timeout, node_exists;

	conn_id = 0;
	if( !Timer_q )
		Timer_q = dtq_create();
	node_exists = get_dns_node_name( nodes );
	if( !(conn_pend_id = find_pend_conn(DNS_TASK, recv_rout)) ) 
	{
		if(!node_exists)
		{
			conn_pend_id = get_free_pend_conn();
			conn_pend = &Pending_conns[conn_pend_id];
/*
			strncpy(conn_pend->node_name, dns_node, MAX_NODE_NAME); 
*/
			strncpy(conn_pend->task_name, DNS_TASK, MAX_TASK_NAME); 
			conn_pend->recv_rout = recv_rout;
			timeout = rand_tmout( tmout_min, tmout_max );
			conn_pend->timr_ent = 
				dtq_add_entry( Timer_q, timeout,
					       retry_dns_connection,
					       conn_pend_id );
			return( -1);
		}
		strcpy(all_nodes, nodes);
		ptr = nodes;			
		while(1)
		{
			dns_node = ptr;
			if(ptr = (char *)strchr(ptr,','))
			{
				*ptr = '\0';			
				ptr++;
			}
			if( conn_id = dna_open_client( dns_node, DNS_TASK, DNS_PORT,
						 TCPIP, recv_rout ))
				break;
			if( !ptr )
				break;
		}
		if(!conn_id)
		{
/*
		sprintf( str,"open_dns : %s on %s \n\ttcpip: Can't open", 
			DNS_TASK, all_nodes );
		net_report_error( 0, -1, str );
*/
			conn_pend_id = get_free_pend_conn();
			conn_pend = &Pending_conns[conn_pend_id];
			strncpy(conn_pend->task_name, DNS_TASK, MAX_TASK_NAME); 
			conn_pend->recv_rout = recv_rout;
			timeout = rand_tmout( tmout_min, tmout_max );
			conn_pend->timr_ent = dtq_add_entry( Timer_q, timeout,
				retry_dns_connection,
				conn_pend_id );
			return( -1);
		}
	}
	return(conn_id);
}


void retry_dns_connection( conn_pend_id )
register int conn_pend_id;
{
	bigbuf char nodes[255];
	bigbuf char str[132];
	register char *dns_node, *ptr;
	register int conn_id, node_exists;
	register PENDING_CONN *conn_pend;
	static int retrying = 0;

	if( retrying ) return;
	retrying = 1;
	conn_pend = &Pending_conns[conn_pend_id];

	node_exists = get_dns_node_name( nodes );
	if(node_exists)
	{
		ptr = nodes;			
		while(1)
		{
			dns_node = ptr;
			if(ptr = (char *)strchr(ptr,','))
			{
				*ptr = '\0';			
				ptr++;
			}
			if( conn_id = dna_open_client( dns_node, conn_pend->task_name,
					 DNS_PORT, TCPIP,
					 conn_pend->recv_rout ) )
				break;
			if( !ptr )
				break;
		}
	}
	if(conn_id)
	{
/*
		sprintf( str,"open_dns : %s on %s \n\ttcpip: Connection established", 
			DNS_TASK, dns_node );
		net_report_error( 0, -1, str );
*/
		rel_pend_conn( conn_pend_id );
		dtq_rem_entry( Timer_q, conn_pend->timr_ent );
	}
	retrying = 0;
}	


static int get_free_pend_conn()
{
	register int i;
	register PENDING_CONN *pending_connp;

	for( i = 1, pending_connp = &Pending_conns[1]; i < MAX_CONNS; 
		i++, pending_connp++ )
	{
		if( pending_connp->task_name[0] == '\0' )
		{
			return(i);
		}
	}
	return(0);
}


static int find_pend_conn( task, rout )
register char *task;
register void (*rout)();
{
	register int i;
	register PENDING_CONN *pending_connp;

	for( i = 1, pending_connp = &Pending_conns[1]; i < MAX_CONNS; 
		i++, pending_connp++ )
	{
		if( (!strcmp(pending_connp->task_name, task)) &&
			(pending_connp->recv_rout == rout) )
		{
			return(i);
		}
	}
	return(0);
}


static rel_pend_conn( conn_id )
register int conn_id;
{
	Pending_conns[conn_id].task_name[0] = '\0';
}	


int rand_tmout( min, max )
int min, max;
{
	int aux;

	aux = rand();
	aux %= (max - min);
	aux += min;
	return(aux);
}
