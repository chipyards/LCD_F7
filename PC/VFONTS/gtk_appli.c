#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkwin32.h>

// #include "modpop.h"

#include "glostru.h"
#include "model.h"
#include "appli.h"

/* inevitables variables globales */

extern flat_tex_quad flat;

#include <stdarg.h>

/* gasp pour windows */

void gasp( char *fmt, ... )  /* fatal error handling */
{
  char lbuf[1024];
  va_list  argptr;
  va_start( argptr, fmt );
  vsprintf( lbuf, fmt, argptr );
  va_end( argptr );
  MessageBox(NULL, lbuf, "HORREUR", MB_OK);
  exit(1);
}


/* creation acces fenetre Win32 */
void acces_GL_in_drawing_area( GtkWidget * area )
{
HWND hWnd;             // window handle
hWnd = (HWND)GDK_WINDOW_HWND( gtk_widget_get_window( area ) );
if	( hWnd == NULL )
	gasp("Echec gtk_widget_get_window()");
prepare_win32_pixel( hWnd);
}

/** ============================ call backs ======================= */

gint close_event_call( GtkWidget *widget,
                        GdkEvent  *event,
                        gpointer   data )
{ return(FALSE); }

void quit_call( GtkWidget *widget, glostru * glo )
{
gtk_widget_destroy( glo->wmain );
}

int idle_call( glostru * glo )
{
if	( glo->darea_queue_flag )
	{
	gtk_widget_queue_draw( glo->darea );
	glo->darea_queue_flag = 0;
	}
/* profileur *
glo->idle_profiler_cnt++;
if	( glo->idle_profiler_time != time(NULL) )
	{
	glo->idle_profiler_time = time(NULL);
	printf( "%3d i %3d e\n", glo->idle_profiler_cnt, glo->expose_cnt );
	glo->idle_profiler_cnt = 0;
	glo->expose_cnt = 0;
	}
//*/
return( -1 );
}

/*
void dest_xy_call( GtkAdjustment *adjustment, glostru * glo )
{
if ( ((int)adjustment->value) != glo->dest_xy )
   {
   glo->dest_xy = (int)adjustment->value;
   glo->darea_queue_flag = 1;
   }
// printf("dest xy = %d now\n", glo->dest_xy );
}

void off_xy_call( GtkAdjustment *adjustment, glostru * glo )
{
if ( ((int)adjustment->value) != glo->off_xy )
   {
   glo->off_xy = (int)adjustment->value;
   glo->darea_queue_flag = 1;
   }
// printf("off xy = %d now\n", glo->off_xy );
}
*/
void k_call( GtkAdjustment *adjustment, glostru * glo )
{
if ( adjustment->value != glo->k )
   {
   glo->k = adjustment->value;
   flat.zf = (int)glo->k;
   glo->darea_queue_flag = 1;
   }
// printf("k = %g now\n", glo->k );
}

void start_call( GtkWidget *widget, glostru * glo )
{
aKey('W');
}

static gboolean expose_call( GtkWidget * widget, GdkEventExpose * event, glostru * glo )
{
// printf("expozed\n");
aDisplay();
swap_win32_buffers();

++glo->expose_cnt;
return FALSE;	// MAIS POURQUOI ???
}

static gboolean configure_call( GtkWidget * widget, GdkEventConfigure * event, glostru * glo )
{
int ww, wh;
gdk_drawable_get_size( widget->window, &ww, &wh );
// printf("configuzed %d x %d\n", ww, wh );
config_viewport( &flat, ww, wh );
return TRUE;	// MAIS POURQUOI ???
}

static gboolean click_call( GtkWidget * widget, GdkEventButton * event, glostru * glo )
{
if	( event->type == GDK_BUTTON_PRESS )
	{
	if	( event->button == 1 )
		aMouse( (int)event->x, (int)event->y, 1 );
	else if	( event->button == 3 )
		aMouse( (int)event->x, (int)event->y, 3 );
	}
else if	( event->type == GDK_BUTTON_RELEASE )
	{
	// ici inserer traitement de la fenetre draguee : action select ou action zoom
	aMouse( (int)event->x, (int)event->y, 0 );
	glo->darea_queue_flag = 1;
	}
/* We've handled the event, stop processing */
return TRUE;
}

static gboolean motion_call( GtkWidget * widget, GdkEventMotion	* event, glostru * glo )
{
GdkModifierType state;

/* le truc de base, si on utilise pas le hint */
state = (GdkModifierType)event->state;

if	( state & GDK_BUTTON1_MASK )
	{
	aMouse( (int)event->x, (int)event->y, 1 );
	glo->darea_queue_flag = 1;
	}
else if	( state & GDK_BUTTON3_MASK )
	{
	aMouse( (int)event->x, (int)event->y, 3 );
	glo->darea_queue_flag = 1;
	}

/* We've handled it, stop processing */
return TRUE;
}

// capture du focus pour les fonctionnement des bindkeys
static gboolean enter_call( GtkWidget * widget, GdkEventCrossing * event, glostru * glo )
{
gtk_widget_grab_focus( widget );
return FALSE;	// We leave a chance to others
}

// evenement bindkey
static gboolean key_call( GtkWidget * widget, GdkEventKey * event, glostru * glo )
{
if	( event->type == GDK_KEY_PRESS )
	{
	int v = event->keyval;
	int s = event->state;
	switch	(v)
		{
		case 0xff52 :	// UP
			if ( s == 0 ) aKey('e'); else aKey('t'); break;
		case 0xff51 :	// LEFT
			if ( s == 0 ) aKey('s'); else aKey('f'); break;
		case 0xff54 :	// DOWN
			if ( s == 0 ) aKey('x'); else aKey('v'); break;
		case 0xff53 :	// RIGHT
			if ( s == 0 ) aKey('d'); else aKey('g'); break;
		default : aKey(v);
		}
//	printf("Key h=%04x v=%04x (%04x) \"%s\" press\n",
//		event->hardware_keycode, event->keyval, event->state, event->string );
//	if	( ( v >= GDK_F1 ) && ( v <= GDK_F12 ) )	// needs gdk/gdkkeysyms.h
//		printf("F%d\n", v + 1 - GDK_F1 );
	glo->darea_queue_flag = 1;
	}
return TRUE;	// We've handled the event, stop processing
}



/** ============================ constr. GUI ======================= */

int main( int argc, char *argv[] )
{
glostru theglo;
#define glo (&theglo)
GtkWidget *curwidg;
const char * bmpnam;

// GError * pgerror;

if	( argc < 2 )
	bmpnam = "bak.bmp";
else	bmpnam = argv[1];

gtk_init(&argc,&argv);

curwidg = gtk_window_new( GTK_WINDOW_TOPLEVEL );

gtk_signal_connect( GTK_OBJECT(curwidg), "delete_event",
                    GTK_SIGNAL_FUNC( close_event_call ), NULL );
gtk_signal_connect( GTK_OBJECT(curwidg), "destroy",
                    GTK_SIGNAL_FUNC( gtk_main_quit ), NULL );

gtk_window_set_title( GTK_WINDOW (curwidg), "Pixu 0.001");
gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 10 );
glo->wmain = curwidg;

/* creer boite verticale */
curwidg = gtk_vbox_new( FALSE, 5 ); /* spacing ENTRE objets */
gtk_container_add( GTK_CONTAINER( glo->wmain ), curwidg );
glo->vmain = curwidg;

/* creer 3 sliders */

	{
	GtkWidget *hbox, *label, *hscale;
	GtkAdjustment *adjustment;

	/*
	hbox = gtk_hbox_new( FALSE, 4 );
	gtk_box_pack_start( GTK_BOX(glo->vmain), hbox, FALSE, FALSE, 0);

	label = gtk_label_new("s1 :");
	gtk_box_pack_start( GTK_BOX(hbox), label, FALSE, FALSE, 0);
	glo->dest_xy = 50;
	adjustment = GTK_ADJUSTMENT( gtk_adjustment_new( glo->dest_xy, 0, 255, 1, 10, 0 ));
	g_signal_connect( adjustment, "value_changed",
			  G_CALLBACK(dest_xy_call), (gpointer)glo );
	hscale = gtk_hscale_new(adjustment);
	gtk_scale_set_digits( GTK_SCALE(hscale), 0 );
	gtk_box_pack_start( GTK_BOX(hbox), hscale, TRUE, TRUE, 0 );
	*/
	/*
	hbox = gtk_hbox_new( FALSE, 4 );
	gtk_box_pack_start( GTK_BOX(glo->vmain), hbox, FALSE, FALSE, 0);

	label = gtk_label_new("offset X et Y :");
	gtk_box_pack_start( GTK_BOX(hbox), label, FALSE, FALSE, 0);

	glo->off_xy = 50;
	adjustment = GTK_ADJUSTMENT( gtk_adjustment_new( glo->off_xy, -100, 155, 1, 10, 0 ));
	g_signal_connect( adjustment, "value_changed",
			  G_CALLBACK(off_xy_call), (gpointer)glo );

	hscale = gtk_hscale_new(adjustment);
	gtk_scale_set_digits( GTK_SCALE(hscale), 0 );
	gtk_box_pack_start( GTK_BOX(hbox), hscale, TRUE, TRUE, 0 );
	*/

	hbox = gtk_hbox_new( FALSE, 4 );
	gtk_box_pack_start( GTK_BOX(glo->vmain), hbox, FALSE, FALSE, 0);

	label = gtk_label_new("skale :");
	gtk_box_pack_start( GTK_BOX(hbox), label, FALSE, FALSE, 0);

	glo->k = 1.0;
	adjustment = GTK_ADJUSTMENT( gtk_adjustment_new( glo->k, 1.0, 10.0, 1.0, 1.0, 0 ));
	g_signal_connect( adjustment, "value_changed",
			  G_CALLBACK(k_call), (gpointer)glo );

	hscale = gtk_hscale_new(adjustment);
	gtk_scale_set_digits( GTK_SCALE(hscale), 0 );
	gtk_box_pack_start( GTK_BOX(hbox), hscale, TRUE, TRUE, 0 );
	}

/* creere une drawing area */
curwidg = gtk_drawing_area_new ();
/* set a minimum size */
gtk_widget_set_size_request (curwidg, 200, 200);

gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, TRUE, TRUE, 0 );


GTK_WIDGET_SET_FLAGS( curwidg, GTK_CAN_FOCUS );
g_signal_connect( curwidg, "expose_event", G_CALLBACK(expose_call), glo );
g_signal_connect( curwidg, "configure_event", G_CALLBACK(configure_call), glo );
g_signal_connect( curwidg, "key_press_event", G_CALLBACK( key_call ), glo );
g_signal_connect( curwidg, "button_press_event",   G_CALLBACK(click_call), glo );
g_signal_connect( curwidg, "button_release_event", G_CALLBACK(click_call), glo );
g_signal_connect( curwidg, "motion_notify_event", G_CALLBACK( motion_call ), glo );
g_signal_connect( curwidg, "enter_notify_event", G_CALLBACK( enter_call ), glo );

// Ask to receive events the drawing area doesn't normally subscribe to
gtk_widget_set_events( curwidg, gtk_widget_get_events(curwidg)
			| GDK_BUTTON_PRESS_MASK
			| GDK_BUTTON_RELEASE_MASK
			| GDK_POINTER_MOTION_MASK
			| GDK_KEY_PRESS_MASK
			| GDK_ENTER_NOTIFY_MASK
		     );
glo->darea = curwidg;

/* creer boite horizontale */
curwidg = gtk_hbox_new( FALSE, 10 ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), 5);
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, FALSE, FALSE, 0 );
glo->hbut = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Write ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( start_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->bsta = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Quit ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( quit_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->bqui = curwidg;

read_bmp_rgba( &flat, bmpnam );

// ajuster la drawing area aux dimensions de la bmp
// c'est pas oblige, en fait, puisqu'on peut la resizer a tout moment
// gtk_widget_set_size_request( glo->darea, flat.tW, flat.tH );
gtk_widget_set_size_request( glo->darea, flat.tW + 2, flat.tH + 2 );
// afficher la drawing area avant d'allumer openGL
gtk_widget_set_double_buffered( glo->darea, FALSE );
gtk_widget_show_all( glo->wmain );

// allumer openGL
acces_GL_in_drawing_area( glo->darea );
// initialiser appli
aInit();

g_timeout_add( 31, (GSourceFunc)(idle_call), (gpointer)glo );

gtk_main();

largue_win32_contexts();
return(0);
}
