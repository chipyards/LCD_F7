
typedef struct
{
int vW, vH;			// dimensions du viewport, seul lien avec les pixels reels
unsigned char * rgbabuf;	// buffer pour la texture
int tW, tH;			// dimensions texture
unsigned int texHand;		// l'id de la texture une fois memorisee chez GL
float xyz3[3*4];		// les 4 vertices du quad, dans un espace 3D
float st2[2*4];			// les coord de texture 2D pour chaque vertex
unsigned vx[4];			// tableau servant a ordonner les vertices
int zf;			// zoom factor pixel/texel
} flat_tex_quad;


void read_bmp_rgba( flat_tex_quad * f, const char * fnam );
void save_crop_bmp_rgba( flat_tex_quad * f, const char * fnam, int x, int y, int w, int h );

void init_quad( flat_tex_quad * f );
// trace du quad en coordonnees ecran
void aff_quad( flat_tex_quad * f, int x, int y );
// trace d'un rectangle de selection en coordonnees ecran, couleur 32 bits rgba
void aff_transp_rect( flat_tex_quad * f, int x0, int y0, int x1, int y1, int color );

// fonction interpretant un resize de la fenetre
void config_viewport( flat_tex_quad * f, int width, int height );
// autorise resampling lineaire
void set_quad_filter( int enable );

// fonctions specifiques de wGL, le wrapper openGL sous win32
// preparation format pixel
void prepare_win32_pixel( HWND win32_win_handle );
// concretiser l'affichage de la frame
void swap_win32_buffers();
// eventuellement tout eteindre
void largue_win32_contexts();
