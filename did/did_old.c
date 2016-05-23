#include <stdio.h>                   
#include <ctype.h>
#include <dic.h>
#include <dim.h>
#include "did.h"

int First_time = 1;
int Curr_view_opt = -1;	
char Curr_view_opt_par[80];	
char Curr_service_name[132];	
int N_servers = 0;	
int N_services = 0;	
int no_link_int = -1;
FILE	*fptr;
char *Service_content_str;
char *Service_buffer;
int Service_size;
int Timer_q;

/*
 * Global data
 */
static XmFontList did_default_font;

static MrmType class_id;		/* Place to keep class ID*/
static MrmType *dummy_class;            /* and class variable. */
#ifndef unix
static char *db_filename_vec[] =        /* Mrm.hierachy file list. */
{
	"delphi$online:[communications.dim.did]did.uid"
};

static int db_filename_num =
                (sizeof db_filename_vec / sizeof db_filename_vec [0]);

#else
static char *db_filename_vec[1];        /* Mrm.hierachy file list. */
static int db_filename_num;
#endif

/*
 * Forward declarations
 */
void did_exit();
void create_main();
void create_label();
void create_matrix();
void view_opts();
void dns_control();
void ok_pop_up();                                                            
void cancel_pop_up();

/*
 * Names and addresses of callback routines to register with Mrm
 */
static MrmRegisterArg reglist [] = {
{"did_exit", (caddr_t)did_exit},
{"create_main", (caddr_t)create_main},
{"create_label", (caddr_t)create_label},
{"create_matrix", (caddr_t)create_matrix},
{"view_opts", (caddr_t)view_opts},
{"dns_control", (caddr_t)dns_control},
{"ok_pop_up", (caddr_t)ok_pop_up},
{"cancel_pop_up", (caddr_t)cancel_pop_up},
};

static int reglist_num = (sizeof reglist / sizeof reglist[0]);

/*
 * OS transfer point.  The main routine does all the one-time setup and
 * then calls XtAppMainLoop.
 */

SERVER *Curr_servp;

unsigned int main(argc, argv)
    unsigned int argc;                  /* Command line argument count. */
    char *argv[];                       /* Pointers to command line args. */
{
	Widget main_window_id;
    Arg arglist[2];
    int n, i;
	char opt_str[20], *ptr;
	static char uid_file[132];

#ifdef unix 

	if( (ptr = getenv("DIMDIR")) == NULL )
	{
		printf("Environment variable DIMDIR not defined\n");
		exit(0);
	}
	strcpy(uid_file,ptr);
	strcat(uid_file,"/did/");
	if( (ptr = getenv("XDIR")) == NULL )
	{
		printf("Environment variable XDIR not defined\n");
		exit(0);
	}
	strcat(uid_file,ptr);
	strcat(uid_file,"/did.uid");
	db_filename_vec[0] = uid_file;
	db_filename_num = 1;
#endif

	if(argc > 1)
	{
		if(argv[1][0] == '/')
		{
			sprintf(opt_str,"%s",&argv[1][1]);
			if((!strncmp(opt_str,"node",4)) || (!strncmp(opt_str,"NODE",4)))
				Curr_view_opt = 0;
			else if((!strncmp(opt_str,"all",3)) || (!strncmp(opt_str,"ALL",3)))
				Curr_view_opt = 1;
			else if((!strncmp(opt_str,"service",7)) || 
					(!strncmp(opt_str,"SERVICE",7)))
				Curr_view_opt = 2;
			else if((!strncmp(opt_str,"error",5)) ||
					(!strncmp(opt_str,"ERROR",5)))
				Curr_view_opt = 3;
			else
				Curr_view_opt = -1;
			if((Curr_view_opt == 0) || (Curr_view_opt == 2))
			{
  				if(!(ptr = strchr(argv[1],'=')))
				{
					if(ptr = strchr(argv[2],'='))
					{
						ptr++;
						if(!(*ptr))
							ptr = argv[3];
					}
					else
						ptr++;
				}
				else
				{			
					ptr++;
					if(!(*ptr))
						ptr = argv[2];
				}
				for(i = 0;*ptr; ptr++, i++)
					Curr_view_opt_par[i] = toupper(*ptr);
				Curr_view_opt_par[i] = '\0';
			}
		}
	}
    MrmInitialize();			/* Initialize MRM before initializing */
                                        /* the X Toolkit. */
/* Initialize additional DEC widgets */
/*    DXmInitialize();			
*/
    /* 
     * If we had user-defined widgets, we would register them with Mrm.here. 
     */

    /* 
     * Initialize the X Toolkit. We get back a top level shell widget.
     */
    XtToolkitInitialize();

    app_context = XtCreateApplicationContext();
    display = XtOpenDisplay(app_context, NULL, argv[0], "example",
                            NULL, 0, &argc, argv);
    if (display == NULL) 
	{
        fprintf(stderr, "%s:  Can't open display\n", argv[0]);
        exit(1);
	}

    n = 0;
    XtSetArg(arglist[n], XmNallowShellResize, True);  n++;
    toplevel_widget = XtAppCreateShell(argv[0], NULL, applicationShellWidgetClass,
                              display, arglist, n);

    /* 
     * Open the UID files (the output of the UIL compiler) in the hierarchy
     */
    if (MrmOpenHierarchy(db_filename_num, /* Number of files. */
      db_filename_vec,                    /* Array of file names.  */
      NULL,                               /* Default OS extenstion. */
      &s_MrmHierarchy)                    /* Pointer to returned MRM ID */
      !=MrmSUCCESS)
        s_error("can't open hierarchy");

	MrmRegisterNames (reglist, reglist_num);

	if (MrmFetchWidget (s_MrmHierarchy,
        	               "main_window",
            	           toplevel_widget,
                	       &main_window_id,
                    	   &class_id )
                       	   != MrmSUCCESS)
	printf ("can't fetch main_window widget\n");

	XtManageChild(main_window_id);
    XtRealizeWidget(toplevel_widget);


    /* 
     * Sit around forever waiting to process X-events.  We never leave
     * XtAppMainLoop. From here on, we only execute our callback routines. 
     */
	app_initialize();
    XtAppMainLoop(app_context);
}

static char no_link = -1;

app_initialize(tag)
int tag;
{
int check_put_label();

void update_servers();

	create_service_contents();
	create_print();
	dic_info_service("DIS_DNS/SERVER_INFO",MONITORED,0,0,0,update_servers,0,
						&no_link,1);                                        

	dtq_add_entry(Timer_q, 2, check_put_label, 0);
}

/*
 * All errors are fatal.
 */
void s_error(problem_string)
    char *problem_string;
{
    printf("%s\n", problem_string);
    exit(0);
}

void did_exit (w, tag, reason)
Widget		w;
int		*tag;
unsigned long	*reason;
{
	exit(0);
}

extern Pixel rgb_colors[MAX_COLORS];

void create_main (w, tag, reason)
Widget		w;
int		*tag;
unsigned long	*reason;
{
Pixmap did_icon;
MrmCode type;

	Window_id = w;
	get_all_colors(s_MrmHierarchy,display,w);
	set_title(toplevel_widget,"DID - DIM Information Display");
	set_icon_title(toplevel_widget,"DID");
    MrmFetchLiteral(s_MrmHierarchy,"did_default_font",
    						display,&did_default_font, &type );


    MrmFetchIconLiteral(s_MrmHierarchy, "did_icon", 
						DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[GREEN], 
						&did_icon);
	set_something(toplevel_widget,XmNiconPixmap,did_icon);

	Timer_q = dtq_create();
/*
	dtq_start_timer(5, app_initialize, 0);
*/
}

void view_opts (w, tag, reason)
Widget		w;
int		*tag;
unsigned long	*reason;
{

	Curr_view_opt = *tag;
	switch(*tag)
	{
		case 0 :
			get_server_node();
			break;
		case 1 :
			show_servers();
			break;
		case 2 :
			get_server_service();
			break;
		case 3 :
			show_servers();
			break;
	}
}

void dns_control (w, tag, reason)
Widget		w;
int		*tag;
unsigned long	*reason;
{

	switch(*tag)
	{
		case 0 :
			dic_cmnd_service("DIS_DNS/PRINT_STATS",0,0);
			break;
		case 1 :
			dic_cmnd_service("DIS_DNS/DEBUG_ON",0,0);
			break;
		case 2 :
			dic_cmnd_service("DIS_DNS/DEBUG_OFF",0,0);
			break;
		case 3 :
			put_selection(2,"confirmation_box","Confirmation");
			break;
		case 4 :
			dic_cmnd_service("DIS_DNS/PRINT_HASH_TABLE",0,0);
			break;
	}
}

get_server_node()
{
Widget id,sel_id;
int i, n_nodes;
char nodes_str[MAX_NODE_NAME*MAX_CONNS];
char *ptr, *node;

	put_selection(0,"node_names","Node Selection");
	sel_id = pop_widget_id[0];
	id = (Widget)XmSelectionBoxGetChild(sel_id,XmDIALOG_HELP_BUTTON);
	XtUnmanageChild(id);
	id = (Widget)XmSelectionBoxGetChild(sel_id,XmDIALOG_APPLY_BUTTON);
	XtUnmanageChild(id);
	id = (Widget)XmSelectionBoxGetChild(sel_id,XmDIALOG_LIST);
	n_nodes = get_nodes(nodes_str);
	ptr = nodes_str;
	for(i=0;i<n_nodes;i++)
	{
		node = ptr;
		ptr = strchr(ptr,'\n');
		*ptr++ = '\0';
		XmListAddItem(id,create_str(node),i+1);
	}
	set_something(sel_id,XmNlistItemCount,i);
	set_something(sel_id,XmNlistVisibleItemCount,(i < 8) ? i : 8);
}	

get_server_service()
{
Widget id,sel_id;
int i, n_nodes;
char nodes_str[MAX_NODE_NAME*MAX_CONNS];
char *ptr, *node;

	put_selection(1,"service_names","Service Selection");
	sel_id = pop_widget_id[1];
	id = (Widget)XmSelectionBoxGetChild(sel_id,XmDIALOG_HELP_BUTTON);
	XtUnmanageChild(id);
	id = (Widget)XmSelectionBoxGetChild(sel_id,XmDIALOG_APPLY_BUTTON);
	XtUnmanageChild(id);
}	

int get_nodes(node_ptr)
char *node_ptr;
{
DNS_SERVER_INFO *ptr;
int i, n_nodes = 0;
SERVER *servp;

	node_ptr[0] = '\0';
	servp = Server_head;
	while(servp = (SERVER *)sll_get_next(servp))
	{
		ptr = &servp->server;
		if(strstr(node_ptr,ptr->node) <= (char *)0)
		{
			strcat(node_ptr,ptr->node);
			strcat(node_ptr,"\n");
			n_nodes++;
		}
	}
	return(n_nodes);
}

void recv_service_info(tag, buffer, size)
int *tag, *size;
int *buffer;
{
char cmnd[132];

	Service_content_str = malloc(512 + (*size)*16);
	Service_buffer = malloc(*size);
	memcpy(Service_buffer, (char *)buffer, *size);
	Service_size = *size;
	if((*size == 4 ) && (*buffer == -1))
	{
		sprintf(Service_content_str,
			"Service %s Not Available\n", Curr_service_name);
	}
	else
	{
		print_service(buffer, ((*size - 1) / 4) + 1);
	}
/*
	if(pop_widget_id[3])
	{
		XtDestroyWidget(pop_widget_id[3]);
		pop_widget_id[3] = 0;
	}
*/
	put_selection(3,"service_content_form","Service Contents");
	set_something(Content_label_id,XmNlabelString, Service_content_str);
}
	
print_service_float(buff, size)
float *buff;
int size;
{
int i;
char str[80];

	sprintf(Service_content_str,
		"Service %s Contents :\n  \n", Curr_service_name);
	for( i = 0; i < size; i++)
	{
		if(i%4 == 0)
		{
			sprintf(str,"%4d:",i);
			strcat(Service_content_str,str);
		}
		sprintf(str,"  %12.6G ",*(buff++));
		strcat(Service_content_str,str);
		if(i%4 == 3)
		{
			strcat(Service_content_str,"\n");
		}
	}
}

print_service_double(buff, size)
double *buff;
int size;
{
int i;
char str[80];

	sprintf(Service_content_str,
		"Service %s Contents :\n  \n", Curr_service_name);
	size /= 2;
	for( i = 0; i < size; i++)
	{
		if(i%4 == 0)
		{
			sprintf(str,"%4d:",i);
			strcat(Service_content_str,str);
		}
		sprintf(str,"  %12.6G ",*(buff++));
		strcat(Service_content_str,str);
		if(i%4 == 3)
		{
			strcat(Service_content_str,"\n");
		}
	}
}

print_service(buff, size)
int *buff, size;
{
int i,j, str_flag = 0;
char *asc, *ptr, str[80];
int last[4];

	sprintf(Service_content_str,
		"Service %s Contents :\n  \n", Curr_service_name);
	asc = (char *)buff;
	for( i = 0; i < size; i++)
	{
		if(i%4 == 0)
		{
			sprintf(str,"%4d: ",i);
			strcat(Service_content_str,str);
		}
		if(!(i%4))
			strcat(Service_content_str,"H");
		sprintf(str,"   %08X ",buff[i]);
		strcat(Service_content_str,str);
		last[i%4] = buff[i];
		if(i%4 == 3)
		{
			strcat(Service_content_str,"   '");
			for(j = 0; j <16; j++)
			{
				if(isprint(asc[j]))
				{
					sprintf(str,"%c",asc[j]);
					strcat(Service_content_str,str);
				}
				else
				{
					sprintf(str,".");
					strcat(Service_content_str,str);
				}
			}
			strcat(Service_content_str,"'\n");
			for(j = 0; j <4; j++)
			{
				if(j == 0)
					strcat(Service_content_str,"      D");
				sprintf(str,"%11d ",last[j]);
				strcat(Service_content_str,str);
			}
			strcat(Service_content_str,"\n");
			asc = (char *)&buff[i+1];
		}
	}
	if(i%4)
	{
			for(j = 0; j < 4 - (i%4); j++)
				strcat(Service_content_str,"            ");
			strcat(Service_content_str,"   '");
			for(j = 0; j < (i%4) * 4; j++)
			{
				if(isprint(asc[j]))
				{
					sprintf(str,"%c",asc[j]);
					strcat(Service_content_str,str);
				}
				else
					strcat(Service_content_str,".");
			}
			strcat(Service_content_str,"'\n");
			for(j = 0; j < (i%4); j++)
			{
				if(j == 0)
					strcat(Service_content_str,"      D");
				sprintf(str,"%11d ",last[j]);
				strcat(Service_content_str,str);
			}
			strcat(Service_content_str,"\n");
	}
}

void ok_pop_up (w, tag, reason)
Widget		w;
int		*tag;
unsigned long	*reason;
{
DNS_SERVER_INFO *ptr;
DNS_SERVICE_INFO *service_ptr;
Widget id, sel_id;
char *str, *pstr, txt_str[2048];
void recv_service_info();
FILE *fptr;
int i;

	if(*tag == 5)
	{
		id = (Widget)XmSelectionBoxGetChild(w,XmDIALOG_TEXT);
		str = (char *)XmTextGetString(id);
		if(!str[0])
		{
			XtFree(str);
			return;
		}
		if( ( fptr = fopen( str, "w" ) ) == (FILE *)0 )
		{
    		printf("Cannot open: %s for writing\n",str);
			return;
		}                   
		ptr = &Curr_servp->server;
		fprintf(fptr,"Server %s on node %s\n    provides %d services :\n",
								ptr->task, ptr->node, ptr->n_services);
		service_ptr = Curr_servp->service_ptr;
		for(i=0;i<ptr->n_services; i++)
		{
			sprintf(str,service_ptr->name);
			fprintf(fptr,"        %s\n",service_ptr->name);
			service_ptr++;
		}		
		fclose(fptr);
		XtFree(str);
		XtUnmapWidget(XtParent(pop_widget_id[4]));
		return;
	}
	if(*tag == 4)
	{
		put_selection(4, "print","Printing...");
		sel_id = pop_widget_id[4];
		id = (Widget)XmSelectionBoxGetChild(sel_id,XmDIALOG_HELP_BUTTON);
		XtUnmanageChild(id);

		id = (Widget)XmSelectionBoxGetChild(sel_id,XmDIALOG_APPLY_BUTTON);
		XtUnmanageChild(id);
		id = (Widget)XmSelectionBoxGetChild(sel_id,XmDIALOG_TEXT);
		str = (char *)XmTextGetString(id);
		if(!str[0])
		{
			XtFree(str);
			return;
		}
		ptr = &Curr_servp->server;
		if(pstr = strrchr(str,']'))
			*(++pstr) = '\0';
		if(pstr = strrchr(str,'/'))
			*(++pstr) = '\0';
		sprintf(txt_str,"%s%s.TXT",str,ptr->task);
		XtFree(str);
		XmTextSetString(id, txt_str);
		return;
	}
	if(*tag == 2)
	{
		dic_cmnd_service("DIS_DNS/KILL_SERVERS",0,0);
		return;
	}
	id = (Widget)XmSelectionBoxGetChild(w,XmDIALOG_TEXT);
	str = (char *)XmTextGetString(id);
	if(!str[0])
	{
		XtFree(str);
		return;
	}
	if(*tag != 3)
	{
		strcpy(Curr_view_opt_par, str);
		show_servers();
		XtFree(str);
	}
	else
	{
		strcpy(Curr_service_name, str);
		dic_info_service(str,ONCE_ONLY,20,0,0,recv_service_info,0,
			&no_link_int,4);
		XtFree(str);
	}
}

void cancel_pop_up (w, tag, reason)
Widget		w;
int		*tag;
unsigned long	*reason;
{
	if(*tag == 6)
	{
		print_service_float(Service_buffer, ((Service_size - 1) / 4) + 1);
		set_something(Content_label_id,XmNlabelString, Service_content_str);
	}
	else if(*tag == 7)
	{
		print_service_double(Service_buffer, ((Service_size - 1) / 4) + 1);
		set_something(Content_label_id,XmNlabelString, Service_content_str);
	}
	else if(*tag == 8)
	{
		print_service(Service_buffer, ((Service_size - 1) / 4) + 1);
		set_something(Content_label_id,XmNlabelString, Service_content_str);
	}
	else if(*tag == 5)
	{
		XtUnmapWidget(XtParent(pop_widget_id[4]));
	}
	else if(*tag == 4)
	{
/*
		XtDestroyWidget(XtParent(XtParent(w)));
		pop_widget_id[3] = 0;
*/
		XtUnmapWidget(XtParent(pop_widget_id[3]));
		free(Service_content_str);
		free(Service_buffer);
	}
}

void create_matrix(w, tag, reason)
Widget		w;
int		*tag;
unsigned long	*reason;
{

	Matrix_id[*tag] = w;
	if(*tag)
		XtUnmanageChild(w);
	else
		Curr_matrix = 0;
}

void create_label(w, tag, reason)
Widget		w;
int		*tag;
unsigned long	*reason;
{

	if(!*tag)
		Label_id = w;
	else
		Content_label_id = w;
}

switch_matrix()
{
	int width = 0, height = 0;

	XtUnmanageChild(Matrix_id[Curr_matrix]);
	Curr_matrix = (Curr_matrix) ? 0 : 1;
	XtManageChild(Matrix_id[Curr_matrix]);
	XmScrolledWindowSetAreas(Window_id,NULL, NULL, Matrix_id[Curr_matrix]); 
}

static int curr_allocated_size = 0;
static DNS_SERVER_INFO *dns_info_buffer;

void get_servers(tag, buffer, size)
int tag,size;
DNS_SERVER_INFO *buffer;
{
}

void update_servers(tag, buffer, size)
int *tag,*size;
DNS_DID *buffer;
{
int n_services, service_size;
SERVER *servp;
Widget create_button();
DNS_SERVER_INFO *server_ptr;
DNS_SERVICE_INFO *service_ptr;
int i, j, found = 0;
Widget w;
char str[MAX_NAME];


	if(!Server_head)
	{
		Server_head = (SERVER *)malloc(sizeof(SERVER));
		sll_init(Server_head);
	}
	if(First_time)
	{
		switch_matrix();
		First_time = 0;
	}
	if(*(char *)buffer == -1)
	{
		remove_all_buttons();
		if(! No_link_button_id)
		{
			No_link_button_id = create_button("DNS is down", 0);
			XtSetSensitive(No_link_button_id, False);
		}
		while(!sll_empty(Server_head))
		{
			servp = (SERVER *)sll_remove_head(Server_head);
			if(servp->service_ptr)
				free(servp->service_ptr);
			free(servp);
		}
		N_servers = 0;
		N_services = 0;
		return;
	}
	if(No_link_button_id)
	{
		XtDestroyWidget(No_link_button_id);
		XFlush(XtDisplay(No_link_button_id));
		No_link_button_id = 0;
    }
	buffer->server.n_services = vtohl(buffer->server.n_services);
	n_services = buffer->server.n_services;
	if(!(servp = (SERVER *)sll_search(Server_head, &buffer->server,
									MAX_TASK_NAME+MAX_NODE_NAME)))
	{
		if(n_services)
		{
			if(n_services == -1)
				n_services = 0;
			N_servers++;
			N_services += n_services;
			servp = (SERVER *)malloc(sizeof(SERVER));
			memcpy(&servp->server,&buffer->server,sizeof(DNS_SERVER_INFO));
			service_size = n_services*sizeof(DNS_SERVICE_INFO);
			if(service_size)
			{
				servp->service_ptr = (DNS_SERVICE_INFO *)malloc(service_size);
				for(j = 0; j < n_services; j++)
				{
					buffer->services[n_services].type = vtohl(
						buffer->services[n_services].type);
					buffer->services[n_services].status = vtohl(
						buffer->services[n_services].status);
					buffer->services[n_services].n_clients = vtohl(
						buffer->services[n_services].n_clients);
				}
				memcpy(servp->service_ptr, buffer->services, service_size);
			}
			else
				servp->service_ptr = 0;
			servp->next = 0;
			servp->button_id = 0;
			servp->pop_widget_id[0] = 0;
			servp->pop_widget_id[1] = 0;
			servp->busy = 0;
			sll_insert_queue(Server_head,servp);
			server_ptr = &servp->server;
			switch(Curr_view_opt)
			{
				case 1 :
					servp->button_id = create_button(server_ptr->task, servp);
					break;
				case 0 :
					if(!strcmp(server_ptr->node, Curr_view_opt_par))
						servp->button_id = create_button(server_ptr->task, servp);
					break;
				case 2 :
					if(!(service_ptr = servp->service_ptr))
						break;
					for(j = 0; j < server_ptr->n_services; j++)
					{
						if(strstr(service_ptr->name, Curr_view_opt_par) > 
							(char *) 0)
						{
							found = 1;
							break;
						}
						service_ptr++;
					}
					if (found)
						servp->button_id = create_button(server_ptr->task,servp);
					break;
				case 3 :
					if(!n_services)
						servp->button_id = create_button(server_ptr->task,servp);
					break;
			}
			if((Curr_view_opt != -1) &&(!n_services))
			{
				if(servp->button_id)
				{
					set_color(servp->button_id, XmNbackground, RED);
					get_something(servp->button_id,XmNuserData,&w);
					set_color(w, XmNbackground, RED);
				}
			}
		}
	}
	else
	{
		if(n_services > 0)
		{
			if(n_services == servp->server.n_services)
			{
				return;
			}
			if(servp->server.n_services != -1)
				N_services -= servp->server.n_services;
			memcpy(&servp->server,&buffer->server,sizeof(DNS_SERVER_INFO));
			service_size = n_services*sizeof(DNS_SERVICE_INFO);
			if(servp->service_ptr)
				free(servp->service_ptr);
			servp->service_ptr = (DNS_SERVICE_INFO *)malloc(service_size);
			memcpy(servp->service_ptr, buffer->services, service_size);
			N_services += n_services;
			if(servp->button_id)
			{
				if(Curr_view_opt == 3)
				{
					XtDestroyWidget(servp->button_id);
					XFlush(XtDisplay(servp->button_id));
					servp->button_id = 0;
				}
				else
				{
					set_color(servp->button_id, XmNbackground, GREEN);
					get_something(servp->button_id,XmNuserData,&w);
					set_color(w, XmNbackground, GREEN);
				}
			}
		}
		else if(n_services == 0)
		{
			N_servers--;
			if(servp->server.n_services != -1)
				N_services -= servp->server.n_services;
			if(servp->service_ptr)
				free(servp->service_ptr);
			sll_remove(Server_head, servp);
			if(servp->button_id)
			{
				XtDestroyWidget(servp->button_id);
				XFlush(XtDisplay(servp->button_id));
				servp->button_id = 0;
			}
			free(servp);
		}
		else if(n_services == -1)
		{
			N_services -= servp->server.n_services;
			servp->server.n_services = -1;
			if(servp->button_id)
			{
				set_color(servp->button_id, XmNbackground, RED);
				get_something(servp->button_id,XmNuserData,&w);
				set_color(w, XmNbackground, RED);
			}
			else
			{
				if(Curr_view_opt == 3)
					servp->button_id = create_button(servp->server.task, servp);
			}
		}
	}
}

show_servers()
{
DNS_SERVER_INFO *server_ptr;
DNS_SERVICE_INFO *service_ptr;
int i, j, found;
Widget create_button();
SERVER *servp;
char str[MAX_NAME];

	if(!Matrix_id[Curr_matrix])
		return;
	remove_all_buttons();
	switch_matrix();
	switch(Curr_view_opt)
	{
		case 1 :
			put_label();			
			servp = Server_head;
			while(servp = (SERVER *)sll_get_next(servp))
			{
				server_ptr = &servp->server;
				servp->button_id = create_button(server_ptr->task, servp);
			}
			break;
		case 0 :
			put_label();
			servp = Server_head;
			while(servp = (SERVER *)sll_get_next(servp))
			{
				server_ptr = &servp->server;
				if(!strcmp(server_ptr->node, Curr_view_opt_par))
					servp->button_id = create_button(server_ptr->task, servp);
			}
			break;
		case 2 :
			put_label();
			servp = Server_head;
			while(servp = (SERVER *)sll_get_next(servp))
			{
				found = 0;
				server_ptr = &servp->server;
				if(!(service_ptr = servp->service_ptr))
					break;
				for(j = 0; j < server_ptr->n_services; j++)
				{
					if(strstr(service_ptr->name, Curr_view_opt_par) > (char *)0)
					{
						found = 1;
						break;
					}
					service_ptr++;
				}
				if (found)
					servp->button_id = create_button(server_ptr->task,servp);
			}
			break;
		case 3 :
			put_label();			
			servp = Server_head;
			while(servp = (SERVER *)sll_get_next(servp))
			{
				server_ptr = &servp->server;
				if(server_ptr->n_services == -1)
				{
					servp->button_id = create_button(server_ptr->task, servp);
				}
			}
			break;
	}
}

Widget create_button(name, servp)
char *name;
SERVER *servp;
{
Arg arglist[10];
int n, n_services = -1;
Widget w, ww, w_id;
void activate_services(), activate_clients();
char w_name[40];
	
	w_name[0] = 0;
	if(servp)
		n_services = servp->server.n_services;
	strcpy(w_name,name);
	n = 0;
    XtSetArg(arglist[n], XmNorientation, XmVERTICAL);  n++;
    XtSetArg(arglist[n], XmNentryAlignment, XmALIGNMENT_CENTER);  n++;
	w_id = w = (Widget)XmCreateMenuBar(Matrix_id[Curr_matrix],
				XmStringCreateLtoR ( w_name,XmSTRING_DEFAULT_CHARSET),
				arglist,n);
	if(n_services == -1)
		set_color(w, XmNbackground, RED);
	else
		set_color(w, XmNbackground, GREEN);
	XtManageChild(w);
	strcat(w_name,"1"); 
	n = 0;
    XtSetArg(arglist[n], XmNalignment, XmALIGNMENT_CENTER);  n++;
	XtSetArg(arglist[n], XmNfontList, did_default_font);  n++;
	w = (Widget)XmCreateCascadeButton(w,
				XmStringCreateLtoR ( w_name,XmSTRING_DEFAULT_CHARSET),
				arglist,n);
	set_something(w,XmNlabelString,name);
	set_something(w,XmNalignment,XmALIGNMENT_CENTER);
	if(n_services == -1)
		set_color(w, XmNbackground, RED);
	else
		set_color(w, XmNbackground, GREEN);
	set_something(w_id,XmNuserData,w);
	strcat(w_name,"1"); 
		n = 0;
		ww = (Widget)XmCreatePulldownMenu(w_id,
				XmStringCreateLtoR ( w_name,XmSTRING_DEFAULT_CHARSET),
				arglist,n);
		set_something(w,XmNsubMenuId,ww);
		XtManageChild(w);
		strcat(w_name,"1"); 
		n = 0;
		XtSetArg(arglist[n], XmNfontList, did_default_font);  n++;
		w = (Widget)XmCreatePushButton(ww,
				XmStringCreateLtoR ( w_name,XmSTRING_DEFAULT_CHARSET),
				arglist,n);

		set_something(w,XmNlabelString,"SERVICES");
	if(servp)
	{
		XtAddCallback(w,XmNactivateCallback, activate_services, servp);
		XtManageChild(w);
		strcat(w_name,"1"); 
		n = 0;
		XtSetArg(arglist[n], XmNfontList, did_default_font);  n++;
		w = (Widget)XmCreatePushButton(ww,
				XmStringCreateLtoR ( w_name,XmSTRING_DEFAULT_CHARSET),
				arglist,n);

		set_something(w,XmNlabelString,"CLIENTS");
		XtAddCallback(w,XmNactivateCallback, activate_clients, servp);
		XtManageChild(w);
		servp->popping = 0;
		create_client_popup(servp);
	}
	return(w_id);
}

remove_all_buttons()
{
SERVER *servp;

	servp = Server_head;
	while(servp = (SERVER *)sll_get_next(servp))
	{
		if(servp->button_id)
		{
			XtDestroyWidget(servp->button_id);
			servp->button_id = 0;
		}
	}
}

void activate_services(w, servp, reason)
Widget		w;
SERVER		*servp;
unsigned long	*reason;
{
DNS_SERVER_INFO *ptr;
DNS_SERVICE_INFO *service_ptr;
char str[2048];
int i;
Widget id,sel_id;

	if(servp->pop_widget_id[0])
	{
		XtDestroyWidget(servp->pop_widget_id[0]);
		servp->pop_widget_id[0] = 0;
		return;
	}
	Curr_servp = servp;
	ptr = &servp->server;


	put_popup(servp, 0,"server_info","Service Info");
	sel_id = servp->pop_widget_id[0];
/*
	id = (Widget)XmSelectionBoxGetChild(sel_id,XmDIALOG_HELP_BUTTON);
	XtUnmanageChild(id);
*/

	id = (Widget)XmSelectionBoxGetChild(sel_id,XmDIALOG_OK_BUTTON);
	XtUnmanageChild(id);


	id = (Widget)XmSelectionBoxGetChild(sel_id,XmDIALOG_TEXT);
	id = (Widget)XmSelectionBoxGetChild(sel_id,XmDIALOG_LIST);

	service_ptr = servp->service_ptr;
	for(i=0;i<ptr->n_services; i++)
	{
		sprintf(str,service_ptr->name);
		service_ptr++;
		XmListAddItem(id,create_str(str),i+1);
	}		
	set_something(sel_id,XmNlistItemCount,i);
	set_something(sel_id,XmNlistVisibleItemCount,(i < 20) ? i : 20);
	id = (Widget)XmSelectionBoxGetChild(sel_id,XmDIALOG_LIST_LABEL);
	sprintf(str,"Server %s on node %s\n\nprovides %d services :\n\n",
								ptr->task, ptr->node, i);
	set_something(sel_id,XmNlistLabelString,str);

}

void show_clients(servp_ptr, buffer, size)
SERVER **servp_ptr;
int *size;
DNS_CLIENT_INFO *buffer;
{
DNS_CLIENT_INFO *dns_client_info;
int i = 0;
char str[2048];
DNS_SERVER_INFO *ptr;
SERVER *servp;
Widget id,sel_id;

/*
    servp = *servp_ptr;
	ptr = &servp->server;
	sprintf(str,"Clients of %s are :\n\n",
								ptr->task);
	dns_client_info = (DNS_CLIENT_INFO *)buffer;
	if(dns_client_info->node[0] == -1) 
	{
		set_something(servp->pop_widget_id[1],XmNmessageString,
			"Information not available\n");
		return;

	}
	while(dns_client_info->node[0]) 
	{
		strcat(str,"Process ");
		strcat(str,dns_client_info->task);
		strcat(str," on node ");
		strcat(str,dns_client_info->node);
		strcat(str,"\n");
		dns_client_info++;
		found = 1;
	}
	if(!found)
		strcat(str,"NONE\n ");

	set_something(servp->pop_widget_id[1],XmNmessageString,str);
*/
	servp = *servp_ptr;
	ptr = &servp->server;
	servp->popping = 1;

	put_popup(servp,1,"client_info","Client Info");

	sel_id = servp->pop_widget_id[1];
	id = (Widget)XmSelectionBoxGetChild(sel_id,XmDIALOG_HELP_BUTTON);
	XtUnmanageChild(id);

	id = (Widget)XmSelectionBoxGetChild(sel_id,XmDIALOG_APPLY_BUTTON);
	XtUnmanageChild(id);

	id = (Widget)XmSelectionBoxGetChild(sel_id,XmDIALOG_TEXT);
	XtUnmanageChild(id);

	id = (Widget)XmSelectionBoxGetChild(sel_id,XmDIALOG_SELECTION_LABEL);
	XtUnmanageChild(id);
/*
	XtUnmanageChild(id);
*/
	id = (Widget)XmSelectionBoxGetChild(sel_id,XmDIALOG_LIST_LABEL);
	dns_client_info = (DNS_CLIENT_INFO *)buffer;
	if(dns_client_info->node[0] == -1) 
	{
		sprintf(str,"Information not available\n");
		set_something(sel_id,XmNlistLabelString,str);
		return;
	}
	sprintf(str,"Clients of %s are :\n\n",
								ptr->task);

	set_something(sel_id,XmNlistLabelString,str);

	id = (Widget)XmSelectionBoxGetChild(sel_id,XmDIALOG_LIST);
	XmListDeleteAllItems(id);
	while(dns_client_info->node[0]) 
	{
		sprintf(str,"Process ");
		strcat(str,dns_client_info->task);
		strcat(str," on node ");
		strcat(str,dns_client_info->node);
		XmListAddItem(id,create_str(str),i+1);
		dns_client_info++;
		i++;
	}
	if(!i)
	{
		sprintf(str,"NONE");
		XmListAddItem(id,create_str(str),i+1);
	}
/*
	set_something(sel_id,XmNlistItemCount,i);
*/
}

void activate_clients(w, servp, reason)
Widget		w;
SERVER		*servp;
unsigned long	*reason;
{
DNS_SERVER_INFO *ptr;
char str[100];
int i;
void show_clients();

	ptr = &servp->server;
	if(servp->popping)
	{
		XtUnmanageChild(servp->pop_widget_id[1]);
/*
		XtUnmapWidget(XtParent(servp->pop_widget_id[1]));
		XtDestroyWidget(servp->pop_widget_id[1]);
		servp->pop_widget_id[1] = 0;
*/
		servp->popping = 0;
		return;
	}
	sprintf(str,"%s/CLIENT_INFO",ptr->task);
	dic_info_service(str,ONCE_ONLY,20,0,0,
				show_clients,servp,&no_link,1);
}

create_client_popup(servp)
SERVER *servp;
{
    Widget sel_id, id, aux_id;
	void activate_services(), activate_clients();
	MrmType class_id;		/* Place to keep class ID*/

	if(!servp->pop_widget_id[1])
	{
		MrmFetchWidget(s_MrmHierarchy, "client_info", toplevel_widget, &sel_id, 
		    &class_id);
		servp->pop_widget_id[1] = sel_id;
		
		XtAddCallback(sel_id,XmNokCallback, activate_clients, servp);
		XtAddCallback(sel_id,XmNcancelCallback, activate_clients, servp);
	}
#ifdef __alpha
	XtManageChild(sel_id);
	XtUnmanageChild(sel_id);
#endif
/*
	XtSetMappedWhenManaged(XtParent(servp->pop_widget_id[1]),False);
	XtManageChild(servp->pop_widget_id[1]);
	set_title(XtParent(servp->pop_widget_id[1]),"Client Info");
*/
}

int put_popup(servp,type,widget_name,title)
SERVER *servp;
char *widget_name, *title;
int type;
{
	MrmType class_id;		/* Place to keep class ID*/
    Widget id, aux_id;
	void activate_services(), activate_clients();

/*
	if(type == 1)
	{
		XtMapWidget(XtParent(servp->pop_widget_id[1]));
		return(0);
	}
*/
	if(!servp->pop_widget_id[type])
	{
		MrmFetchWidget(s_MrmHierarchy, widget_name, toplevel_widget, &id, 
		    &class_id);
		servp->pop_widget_id[type] = id;
		
		if(type)
		{
			XtAddCallback(id,XmNokCallback, activate_clients, servp);
			XtAddCallback(id,XmNcancelCallback, activate_clients, servp);
		}
		else
		{
/*
			XtAddCallback(id,XmNokCallback, activate_services, servp);
*/
			XtAddCallback(id,XmNcancelCallback, activate_services, servp);
		}
	}
	XtManageChild(servp->pop_widget_id[type]);
	set_title(XtParent(servp->pop_widget_id[type]),title);
}	

int create_service_contents()
{
    Widget id;
	MrmType class_id;		/* Place to keep class ID*/

	MrmFetchWidget(s_MrmHierarchy, "service_content_form", toplevel_widget, 
		&id, &class_id);
	pop_widget_id[3] = id;
	XtSetMappedWhenManaged(XtParent(pop_widget_id[3]),False);
	XtManageChild(pop_widget_id[3]);
	set_title(XtParent(pop_widget_id[3]),"Service Contents");
/*
	XtUnmapWidget(XtParent(pop_widget_id[3]));
*/
}	

int create_print()
{
    Widget id;
	MrmType class_id;		/* Place to keep class ID*/

	MrmFetchWidget(s_MrmHierarchy, "print", toplevel_widget, 
		&id, &class_id);
	pop_widget_id[4] = id;
	XtSetMappedWhenManaged(XtParent(pop_widget_id[4]),False);
	XtManageChild(pop_widget_id[4]);
	set_title(XtParent(pop_widget_id[4]),"Printing");
/*
	XtUnmapWidget(XtParent(pop_widget_id[3]));
*/
}	

int put_selection(tag,widget_name,title)
int tag;
char *widget_name, *title;
{
    Widget id;
	MrmType class_id;		/* Place to keep class ID*/

	if((tag == 3)||(tag == 4))
	{
		XtMapWidget(XtParent(pop_widget_id[tag]));
		return(0);
	}
	if(!pop_widget_id[tag])
	{
		MrmFetchWidget(s_MrmHierarchy, widget_name, toplevel_widget, &id, 
		    &class_id);
		pop_widget_id[tag] = id;
	}
	XtManageChild(pop_widget_id[tag]);
	set_title(XtParent(pop_widget_id[tag]),title);
}	

check_put_label(tag)
int tag;
{
	static int old_n_services = 0;

	if(N_services != old_n_services)
	{
		put_label();
		old_n_services = N_services;
	}
}

put_label()
{
	char str[MAX_NAME], str1[MAX_NAME];
			
	DISABLE_AST
	sprintf(str,"%d Servers known - %d Services Available\n",
		N_servers,N_services);
	switch(Curr_view_opt)
	{
		case 1 :
			strcat(str,"Displaying ALL Servers");
			break;
		case 0 :
			sprintf(str1,"Displaying Servers on node %s",Curr_view_opt_par);
			strcat(str,str1);
			break;
		case 2 :
			sprintf(str1,"Displaying Servers providing Service *%s*",
				Curr_view_opt_par);
			break;
		case 3 :
			strcat(str,"Displaying Servers in ERROR");
			break;
		case -1 :
			strcat(str,"Please Select Viewing Option");
			break;
	}
	set_something(Label_id,XmNlabelString,str);
	ENABLE_AST
}
