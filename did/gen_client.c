#include <ctype.h>
#include <stdio.h>
#include <dic.h>

int no_link = 0xcafefead;
int version;
	
char str[80];
int type, mode;

rout(tag, buffer, size)
int *tag, *size;
int *buffer;
{
int i, j;
int hexval;
double res;
char asc[4];

	if((*size == 4 ) && (*buffer == no_link))
	{
		printf("Service %s Not Available\n", str);
		fflush(stdout);
		sys$wake(0,0);
	}
	else
	{
		printf("Service %s Contents :\n", str);
		print_service(buffer, ((*size - 1)/4) + 1);
	}
}

main(argc,argv)
int argc;
char **argv;                    
{
	int time;

	if(argc < 2)
	{
		printf("Service Name > ");
		fflush(stdout);
		scanf("%s", str);
	}
	else
	{
		sprintf(str,argv[1]);
	}
/*
	printf("Service Type (0:integer, 1:real, 2:string) > ");
	scanf("%d",&type);
	printf("Service Size (in bytes) > ");
	scanf("%d",&size);
	printf("Update Mode (0:TIMED, 1:MONITORED) > ");
	fflush(stdout);
	scanf("%d",&mode);
	if(!mode)
	{
		printf("Time interval (in seconds) > ");
		fflush(stdout);
		scanf("%d",&time);
		mode = TIMED;
	}
	else
	{
		mode = MONITORED;
		time = 0;
	}
*/

/*
	dic_info_service(str,mode,time,0,0,rout,0,&no_link,4);
*/
	dic_info_service(str,ONCE_ONLY,60,0,0,rout,0,&no_link,4);
/*
	while(1)
	{
		sleep(10);
	}
*/
	sys$hiber();
}

print_service(buff, size)
int *buff, size;
{
int i,j, str_flag = 0;
char *asc, *ptr;
	int last[4];

/*
	ptr = buff;
	asc = (char *) 0;
	for( i = 0; i < size*4;)
	{
*/
/*
		if(!asc)
			asc = ptr;
		str_flag = 1;
		if(!isprint(asc[0]))
			str_flag = 0;
		if(str_flag)
		{
			for(j = 0; asc[j]; j++)
				if(!isprint(asc[j]))
					str_flag = 0;
		}
		if(str_flag)
		{
			printf("%s\n",asc);
			i += j+1;
			ptr += j+1;
			asc = &asc[j+1];
		}
		else
		{
*/
/*
			asc = (char *) 0;
			printf("%14.2f ", (float) *(float *)ptr);
			printf("%11d ", (int) *(int *)ptr);
			printf("%08X\n", (int) *(int *)ptr);
			ptr += 4;
			i += 4;
*/
/*
		}
*/
/*
	}
*/
	asc = buff;
	for( i = 0; i < size; i++)
	{
		if(!(i%4))
			printf("H");
		printf("   %08X ",buff[i]);
		last[i%4] = buff[i];
		if(i%4 == 3)
		{
			printf("    '");
			for(j = 0; j <16; j++)
			{
				if(isprint(asc[j]))
					printf("%c",asc[j]);
				else
					printf(".");
			}
			printf("'\n");
			for(j = 0; j <4; j++)
			{
				if(j == 0)
					printf("D");
				printf("%11d ",last[j]);
			}
			printf("\n");
			asc = &buff[i+1];
		}
	}
	if(i%4)
	{

			for(j = 0; j < 4 - (i%4); j++)
				printf("            ");
			printf("    '");
			for(j = 0; j < (i%4) * 4; j++)
			{
				if(isprint(asc[j]))
					printf("%c",asc[j]);
				else
					printf(".");
			}
			printf("'\n");
			for(j = 0; j < (i%4); j++)
			{
				if(j == 0)
					printf("D");
				printf("%11d ",last[j]);
			}
			printf("\n");
	}
	fflush(stdout);
	sys$wake(0,0);
}
