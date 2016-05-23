#ifndef __DISDEFS
#define __DISDEFS
#ifndef OSK
#	ifdef _OSK
#		define OSK
#	endif
#endif

/* CFORTRAN interface */
/*
#define dis_start_serving dis_start_serving_
#define dis_get_next_cmnd dis_get_next_cmnd_
#define dis_add_service dis_add_service_
#define dis_add_cmnd dis_add_cmnd_
#define dis_report_service dis_report_service_
#define dis_update_service dis_update_service_
#define dis_remove_service dis_remove_service_
#define dis_send_service dis_send_service_
#define dis_convert_str dis_convert_str_
*/
/* Service type definition */

#ifndef ONCE_ONLY
#define ONCE_ONLY	0x01
#define TIMED		0x02
#define MONITORED	0x04
#define COMMAND		0x08
#define DELETE		0x10
#define MONIT_ONLY	0x20
#define UPDATE 		0x40
#define TIMED_ONLY	0x80
#define MAX_TYPE_DEF 0x80
#endif

/* Routine definition */
#if 0 /* Comment out this proto hack */
#ifndef _PROTO
#ifndef OSK		/* Temorary hack */
#	if (__STDC__ == 1) || defined(ultrix) 
#		define	_PROTO(func,param)	func param
#	else
#		define _PROTO(func,param)	func ()
#	endif
#else
#	define _PROTO(func,param)	func ()
#endif
#endif
#endif /* End of comment */
int dis_start_serving(char *task_name);
int dis_get_next_cmnd(int *tag, int *buffer, int *size );
unsigned dis_add_service(char *service_name, char *service_type,
				   void *service_address, int service_size,
				   void (*usr_routine)(), int tag);
void dis_add_cmnd(char *service_name, char *service_type,
			           void (*usr_routine)(), int tag);
void dis_report_service(char *service_name);
int dis_update_service(unsigned service_id);
int dis_remove_service(unsigned service_id);
void dis_send_service(unsigned service_id, int *buffer,
				   int size);

#endif
