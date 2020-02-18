
#define INIT_CHUNK_SPACE 4096

// un buffer pour download de fichier via curl
class curlchunk {
public :
string ade_host;
string ade_path;
string login_pwd;
string projectid;
string sessionid;
char *data;
size_t size;	// taille data SANS compter le null final
size_t space;	// espace alloue
// constructeur
curlchunk() : data(NULL), size(0), space(0) {
  data = (char *)malloc( INIT_CHUNK_SPACE );	// alloc initiale, va croitre exponentiellement
  if	( data )
	{
	space = INIT_CHUNK_SPACE;
	data[0] = 0;
	}
  };
// methodes
void check_alloc( size_t newsize );	// reallocation exponentielle
void opensession();	// ouverture de session, utilise projectid, renseigne sessionid
void closesession();	// detruit sessionid
int download( const char * fullurl );
int download( string query_string );	// utilise ADE_HOST et ADE_PATH
// cette fonction s'assure de l'existence d'un fichier xml associe a un funcode,
// le telechargeant si necessaire ou si force
// appelle download le cas echeant, precede de opensession si necessaire, retour 0 si ok
// int grab_xml( string funcode, int force_download );
string grab_xml( string & xmlpath, string & funcode, int download_flag );
};

// generation d'un nom de fichier xml
// void funcode2fnam( string funcode, char * fnam, int size );
