#include <dis.h>
#include <dic.h>

char str_srv[80];
int buff_cli[20];

int no_link = -1;

void rout( tag, size )
int *tag, *size;
{

	if(buff_cli[0] == -1)
	{
		printf("No Link for service %d\n",*tag);
	}
}

main(argc,argv)
int argc;
char **argv;
{
	int i, *ptr;
	char aux[80];

	sprintf(str_srv,"hello world");
	sprintf(aux,"%s/MY_TEST",argv[1]);
	dis_add_service( aux, "C", str_srv, 12, 
		(void *)0, 0 );

	dis_start_serving( argv[1] );

	dic_info_service( "NON_EXISTENT_SERVICE_1", MONITORED, 0, 
		buff_cli, 80, rout, 1, &no_link, 4 );

	dic_info_service( "NON_EXISTENT_SERVICE_2", MONITORED, 0, 
		buff_cli, 80, rout, 2, &no_link, 4 );

	while(1) 
	{
		pause();
	} 
}

