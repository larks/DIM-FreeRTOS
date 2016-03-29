#ifndef __DICDEFS
#define __DICDEFS

#ifndef OSK
#	ifdef _OSK
#		define OSK
#	endif
#endif


/* part for CFORTRAN */

#define dic_info_service dic_info_service_
#define dic_cmnd_service dic_cmnd_service_
#define dic_change_address dic_change_address_
#define dic_release_service dic_release_service_
#define dic_find_service dic_find_service_

/* Service type definition */

#ifndef ONCE_ONLY
#define ONCE_ONLY	0x01
#define TIMED		0x02
#define MONITORED	0x04
#define COMMAND		0x08
#define DELETE		0x10
#define MONIT_ONLY	0x20
#define UPDATE		0x40
#define TIMED_ONLY	0x80
#endif

/* Routine definition */

#ifndef _PROTO
#ifndef OSK		/* Temorary hack */
#	if (__STDC__ == 1)/* || defined(ultrix) */
#		define	_PROTO(func,param)	func param
#	else
#		define _PROTO(func,param)	func ()
#	endif
#else
#	define _PROTO(func,param)	func ()
#endif
#endif

_PROTO( unsigned dic_info_service, (char *service_name, int req_type,
				    int req_timeout, void *service_address,
				    int service_size, void (*usr_routine)(),
				    int tag, void *fill_addr, int fill_size) );
_PROTO( int dic_cmnd_service,      (char *service_name, void *service_address,
				    int service_size) );
_PROTO( void dic_change_address,  (unsigned service_id, void *service_address,
				    int service_size) );
_PROTO( void dic_release_service,  (unsigned service_id) );
_PROTO( int dic_find_service,      (char *service_name) );

#endif
