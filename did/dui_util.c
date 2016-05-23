/*
**++
**  FACILITY:  DUI
**
**  MODULE DESCRIPTION:
**
**      Implements MOTIF utility functions
**
**  AUTHORS:
**
**      C. Gaspar
**
**  CREATION DATE:  24-01-1993
**
**--
*/
#include <stdio.h>
#include <Mrm/MrmAppl.h>                   /* Motif Toolkit */
#include <Xm/Xm.h>

#include "dui_util.h"

/* compound strings */


static XmString Active_str;

XmString get_str(text)
char *text;
{

	Active_str = XmStringCreateLtoR ( text, XmSTRING_DEFAULT_CHARSET); 
	return(Active_str);
}


free_str()
{

	XmStringFree(Active_str);
}


XmString create_str(text)
char *text;
{
XmString str;


	str = XmStringCreate ( text, XmSTRING_DEFAULT_CHARSET); 
	return(str);
}

delete_str(str)
XmString str;
{

	XmStringFree(str);
}

void set_something(w, resource, value)
    Widget w;
    char *resource, *value;
{
    Arg al[1];
	int free = 0;
	if( (!strcmp(resource,XmNlabelString)) ||
		(!strcmp(resource,XmNmessageString)) ||
		(!strcmp(resource,XmNtextString)) ||
		(!strcmp(resource,XmNokLabelString)) ||
		(!strcmp(resource,XmNapplyLabelString)) ||
		(!strcmp(resource,XmNcancelLabelString)) ||
		(!strcmp(resource,XmNlistLabelString)) ||
		(!strcmp(resource,XmNselectionLabelString)) )
	{
		free = 1;
		value = (char *)get_str(value);
	}
    XtSetArg(al[0], resource, value);
    XtSetValues(w, al, 1);
	if(free)
		free_str();
	XFlush(XtDisplay(w));
}

void get_something(w, resource, value)
    Widget w;
    char *resource, *value;
{

    Arg al[1];
	int free = 0;
	XmString str;
	char *cstr;

	if( (!strcmp(resource,XmNlabelString)) ||
		(!strcmp(resource,XmNmessageString)) ||
		(!strcmp(resource,XmNtextString)) ||
		(!strcmp(resource,XmNlistLabelString)) ||
		(!strcmp(resource,XmNselectionLabelString)) )
	{
		free = 1;
	    XtSetArg(al[0], resource, &str);
	}
	else
		XtSetArg(al[0], resource, value);
    XtGetValues(w, al, 1);
	if(free)
	{
		XmStringGetLtoR(str, XmSTRING_DEFAULT_CHARSET, &cstr);
		strcpy(value,cstr);
		XtFree(cstr);
	}
}

void set_something_uid(hid, w, resource, value)
    MrmHierarchy hid;
    Widget w;                          
    char *resource, *value;
{
    Arg al[1];

	XtSetArg(al[0], resource, value);
/*
    al[0].name = resource;
    al[0].value = value;
*/

    MrmFetchSetValues(hid, w, al, 1);
}


static XmString str_table[50];

XmStringTable create_str_table(strs)
char strs[50][256];
{
int i;

	for(i=0;strs[i][0];i++)
	{
		str_table[i] = XmStringCreate ( strs[i], XmSTRING_DEFAULT_CHARSET);
	}
	str_table[i] = (XmString)0;
	return((XmStringTable)str_table);
}

del_str_table()
{
int i;

	for(i=0;str_table[i];i++)
		XmStringFree(str_table[i]);

}

 Pixel rgb_colors[MAX_COLORS];
static Pixmap pixmap_colors[MAX_COLORS];
static Pixmap watch_colors[MAX_COLORS];
static Pixmap locks[MAX_COLORS];
static Pixmap unlock;
static Pixmap faces[MAX_COLORS];

Pixel get_named_color(color)
int color;
{
	return(rgb_colors[color]);
}

set_color(w, resource, color) 
Widget w;
char *resource;
int color;
{

/*
	if(resource == XmNbackgroundPixmap)
*/
	if(!strcmp(resource,XmNbackgroundPixmap))
		set_something(w,resource,pixmap_colors[color]);
	else
		set_something(w,resource,rgb_colors[color]);
}
	
set_watch(w, color) 
Widget w;
int color;
{
	set_something(w,XmNbackgroundPixmap,watch_colors[color]);
}
	
set_lock(w, color) 
Widget w;
int color;
{
	set_something(w,XmNbackgroundPixmap,locks[color]);
}
	
set_unlock(w) 
Widget w;
{
	set_something(w,XmNbackgroundPixmap,unlock);
}
	
set_face(w, color) 
Widget w;
int color;
{
	set_something(w,XmNbackgroundPixmap,faces[color]);
}
	
void get_all_colors(hid, display, w)
    MrmHierarchy hid;
	Display *display;
	Widget w;
{
    MrmFetchColorLiteral(hid, "green", display, 0, &rgb_colors[GREEN]);
    MrmFetchColorLiteral(hid, "blue", display, 0, &rgb_colors[BLUE]);
    MrmFetchColorLiteral(hid, "red", display, 0, &rgb_colors[RED]);
    MrmFetchColorLiteral(hid, "yellow", display, 0, &rgb_colors[YELLOW]);
    MrmFetchColorLiteral(hid, "dark_green", display, 0, &rgb_colors[DARK_GREEN]);
    MrmFetchColorLiteral(hid, "dark_red", display, 0, &rgb_colors[DARK_RED]);
    MrmFetchColorLiteral(hid, "black", display, 0, &rgb_colors[BLACK]);
    MrmFetchColorLiteral(hid, "orange", display, 0, &rgb_colors[ORANGE]);
    MrmFetchColorLiteral(hid, "white", display, 0, &rgb_colors[WHITE]);
	get_something(w,XmNbackground,&rgb_colors[NONE]);
}

void get_all_pixmaps(hid, display, w)
    MrmHierarchy hid;
	Display *display;
	Widget w;
{

    MrmFetchIconLiteral(hid, "dui_das_color_icon", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[GREEN], 
						&pixmap_colors[GREEN]);
    MrmFetchIconLiteral(hid, "dui_das_color_icon", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[BLUE], 
						&pixmap_colors[BLUE]);
    MrmFetchIconLiteral(hid, "dui_das_color_icon", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[RED], 
						&pixmap_colors[RED]);
    MrmFetchIconLiteral(hid, "dui_das_color_icon", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[YELLOW], 
						&pixmap_colors[YELLOW]);
    MrmFetchIconLiteral(hid, "dui_das_color_icon", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[NONE], 
						&pixmap_colors[NONE]);
    MrmFetchIconLiteral(hid, "dui_das_color_icon", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[BLACK], 
						&pixmap_colors[BLACK]);

    MrmFetchIconLiteral(hid, "dui_das_watch_icon", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[GREEN], 
						&watch_colors[GREEN]);
    MrmFetchIconLiteral(hid, "dui_das_watch_icon", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[BLUE], 
						&watch_colors[BLUE]);
    MrmFetchIconLiteral(hid, "dui_das_watch_icon", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[RED], 
						&watch_colors[RED]);
    MrmFetchIconLiteral(hid, "dui_das_watch_icon", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[YELLOW], 
						&watch_colors[YELLOW]);
    MrmFetchIconLiteral(hid, "dui_das_watch_icon", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[NONE], 
						&watch_colors[NONE]);
    MrmFetchIconLiteral(hid, "dui_das_watch_icon", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[BLACK], 
						&watch_colors[BLACK]);

    MrmFetchIconLiteral(hid, "dui_das_lock_red", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[NONE], 
						&locks[RED]);
    MrmFetchIconLiteral(hid, "dui_das_lock_green", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[NONE], 
						&locks[GREEN]);
    MrmFetchIconLiteral(hid, "dui_das_lock_blue", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[NONE], 
						&locks[BLUE]);
    MrmFetchIconLiteral(hid, "dui_das_lock_yellow", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[NONE], 
						&locks[YELLOW]);
    MrmFetchIconLiteral(hid, "dui_das_lock_none", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[NONE], 
						&locks[NONE]);
    MrmFetchIconLiteral(hid, "dui_das_unlock", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[NONE], 
						&unlock);
/*
    MrmFetchIconLiteral(hid, "dui_das_unlock_notready", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[NONE], 
						&locks[YELLOW]);
*/
    MrmFetchIconLiteral(hid, "dui_das_running_face", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[GREEN], 
						&faces[GREEN]);
    MrmFetchIconLiteral(hid, "dui_das_ready_face", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[BLUE], 
						&faces[BLUE]);
    MrmFetchIconLiteral(hid, "dui_das_not_ready_face", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[YELLOW], 
						&faces[YELLOW]);
    MrmFetchIconLiteral(hid, "dui_das_error_face", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[RED], 
						&faces[RED]);
    MrmFetchIconLiteral(hid, "dui_das_color_icon", DefaultScreenOfDisplay(display), 
						display, rgb_colors[BLACK], rgb_colors[NONE], 
						&faces[NONE]);
}



static int was_sensitive = 0;

set_sensitive(widget_id)
Widget widget_id;
{

	if(was_sensitive)
		XtSetSensitive(widget_id,True);
}

set_insensitive(widget_id)
Widget widget_id;
{

	if(was_sensitive = XtIsSensitive(widget_id))
		XtSetSensitive(widget_id,False);
}
	
void set_title(w, title)
    Widget w;
    char *title;
{
    Arg al[1];

    XtSetArg(al[0], XmNtitle, title);
    XtSetValues(w, al, 1);
	XFlush(XtDisplay(w));
}

void set_icon_title(w, title)
    Widget w;
    char *title;
{
    Arg al[1];

    XtSetArg(al[0], XmNiconName, title);
    XtSetValues(w, al, 1);
	XFlush(XtDisplay(w));
}

