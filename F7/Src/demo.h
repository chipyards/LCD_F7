// ---------------------- demo des fonts et primitives scrollables --------------------------

#define DEMO_DY 20031	// la demo genere le contenu a la demande, on peut la faire aussi grande qu'on veut ;-)

void draw_r_arrow( int x, int y, int w, int h );

void draw_centered_text( int x, int y, int len, const char * txt );

// tracer une page de demo scrollable verticalement
void demo_draw( int ypos, int xs, int dx );
