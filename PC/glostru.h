/* pseudo global storage ( allocated in main()  )
 
   JLN's GTK widget naming chart :
   w : window
   f : frame
   h : hbox
   v : vbox
   b : button
   l : label
   e : entry
   s : spin adj
   m : menu
   o : option
 */


typedef struct
{
GtkWidget * wmain;
GtkWidget * vmain;
GtkWidget *   hbut;
GtkWidget *   darea;
GtkWidget *     bsta;
GtkWidget *     bqui;


int dest_xy;
int off_xy;
double k;

int darea_queue_flag;

int expose_cnt;
int idle_profiler_cnt;
int idle_profiler_time;
} glostru;

