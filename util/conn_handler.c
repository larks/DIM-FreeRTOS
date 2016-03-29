/*
 * DNA (Delphi Network Access) implements the network layer for the DIM
 * (Delphi Information Managment) System.
 *
 * Started date   : 10-11-91
 * Written by     : C. Gaspar
 * UNIX adjustment: G.C. Ballintijn
 *
 * $Id: conn_handler.c,v 1.1 1994/07/18 19:50:03 gerco Exp gerco $
 */

/* This module can only handle one type of array, and DIC or DNS.
 * It cannot handle both simultaniously. It handles at same time
 * the NET and DNA array's. Although these have to be explicitly
 * created.
 */

#include <dim.h>

static SRC_TYPES My_type;		/* Var. indicating type DIC or DIS */
NOSHARE DNS_CONNECTION *Dns_conns = 0;
NOSHARE DIC_CONNECTION *Dic_conns = 0;
NOSHARE DNA_CONNECTION *Dna_conns = 0;
NOSHARE NET_CONNECTION *Net_conns = 0;

static void **Id_arr;
static int Curr_N_Ids = 0;
NOSHARE int Curr_N_Conns = 0;

void conn_arr_create(type)
SRC_TYPES type;
{
	char *conn_ptr;
	int conn_size;

	if( Curr_N_Conns == 0 )
		Curr_N_Conns = CONN_BLOCK;

	switch(type)
	{
	case SRC_DIC :
		Dic_conns = (DIC_CONNECTION *)
				calloc( Curr_N_Conns, sizeof(DIC_CONNECTION) );
		My_type = type;
		break;
	case SRC_DNS :
		Dns_conns = (DNS_CONNECTION *)
				calloc( Curr_N_Conns, sizeof(DNS_CONNECTION) );
		My_type = type;
		break;
	case SRC_DNA :
		Dna_conns = (DNA_CONNECTION *)
				calloc( Curr_N_Conns, sizeof(DNA_CONNECTION) );
		Net_conns = (NET_CONNECTION *)
				calloc( Curr_N_Conns, sizeof(NET_CONNECTION) );
		break;
	}
}


int conn_get()
{
	register DNA_CONNECTION *dna_connp;
	int i, n_conns, conn_id;

	DISABLE_AST
	for( i = 1, dna_connp = &Dna_conns[1]; i < Curr_N_Conns; i++, dna_connp++ )
	{
		if( !dna_connp->busy )
		{
			dna_connp->busy = TRUE;
			ENABLE_AST
			return(i);
		}
	}
	n_conns = Curr_N_Conns + CONN_BLOCK;
	Dna_conns = arr_increase( Dna_conns, sizeof(DNA_CONNECTION), n_conns );
	Net_conns = arr_increase( Net_conns, sizeof(NET_CONNECTION), n_conns );
	switch(My_type)
	{
	case SRC_DIC :
		Dic_conns = arr_increase( Dic_conns, sizeof(DIC_CONNECTION),
					  n_conns );
		break;
	case SRC_DNS :
		Dns_conns = arr_increase( Dns_conns, sizeof(DNS_CONNECTION),
					  n_conns );
		break;
	}
	conn_id = Curr_N_Conns;
	Curr_N_Conns = n_conns;
	Dna_conns[conn_id].busy = TRUE;
	ENABLE_AST
	return(conn_id);
}


conn_free(conn_id)
int conn_id;
{
	DISABLE_AST
	Dna_conns[conn_id].busy = FALSE;
	ENABLE_AST
}


void *arr_increase(conn_ptr, conn_size, n_conns)
void *conn_ptr;
int conn_size, n_conns;
{
	register char *new_ptr;

	new_ptr = realloc( conn_ptr, conn_size * n_conns );
	memset( new_ptr + conn_size * Curr_N_Conns, 0, conn_size * CONN_BLOCK );
	return(new_ptr);
}

void id_arr_create()
{

	Curr_N_Ids = ID_BLOCK;
	Id_arr = (void *) calloc( Curr_N_Ids, sizeof(void *));
}


void *id_arr_increase(id_ptr, id_size, n_ids)
register void *id_ptr;
register int id_size, n_ids;
{
	register char *new_ptr;

	new_ptr = realloc( id_ptr, id_size * n_ids );
	memset( new_ptr + id_size * Curr_N_Ids, 0, id_size * ID_BLOCK );
	return(new_ptr);
}

int id_get(ptr)
register void *ptr;
{
	register int i, id;
	register void **id_arrp;

	DISABLE_AST
	if(!Curr_N_Ids)
	{
		id_arr_create();
	}
	for( i = 1, id_arrp = &Id_arr[1]; i < Curr_N_Ids; i++, id_arrp++ )
	{
		if( !*id_arrp )
		{
			*id_arrp = ptr;
			ENABLE_AST
			return(i);
		}
	}
	Id_arr = id_arr_increase( Id_arr, sizeof(void *), Curr_N_Ids + ID_BLOCK );

	id = Curr_N_Ids;
	Id_arr[id] = ptr;
	Curr_N_Ids += ID_BLOCK;
	ENABLE_AST
	return(id);
}

void *id_get_ptr(id)
int id;
{

	return(Id_arr[id]);
}


void id_free(id)
int id;
{
	DISABLE_AST
	Id_arr[id] = (void *)0;
	ENABLE_AST
}


