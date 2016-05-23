#include <Mrm/MrmAppl.h>                /* Motif Toolkit and MRM */
#include <Xm/Xm.h>
#include <dui_colors.h>
/* VUIT routines for the user to call */
void s_error();

/* Motif Global variables */
Display         *display;			/* Display variable */
XtAppContext    app_context;		/* application context */
Widget			toplevel_widget;	/* Root widget ID of application */
MrmHierarchy	s_MrmHierarchy;		/* MRM database hierarchy ID */

typedef struct item{
    struct item *next;
	DNS_SERVER_INFO server;
	DNS_SERVICE_INFO *service_ptr;
	Widget button_id;
	Widget pop_widget_id[2];
	int popping;
	int busy;
}SERVER;

SERVER *Server_head = (SERVER *)0;

Widget Matrix_id[2] = {0, 0};
int Curr_matrix;
Widget Label_id = 0;
Widget Content_label_id = 0;
Widget Window_id = 0;
Widget pop_widget_id[5] = {0,0,0,0,0};
Widget No_link_button_id;

