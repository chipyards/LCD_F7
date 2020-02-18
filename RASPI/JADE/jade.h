// options de compilation
#define ADE_COLOR	// prise en compte des couleurs ADE

// valeur max d'ID attendue pour les 3 tables, car on utilise des tableaux de dimension fixe pour des raisons d'efficacite
#define MAXID 1048576	// un bon mega

// gestion du temps
#define MNperSLOT	(15)
#define SECperSLOT	(MNperSLOT*60)
#define SECperDAY	(24*3600)
#define SLOTperDAY	(61)		// 7h45 to 23h00
#define HOURperSLOT	(((double)MNperSLOT)/60.0)

// flags d'options pour les dumps
#define DUMP_WEEK	0x0001
#define DUMP_TIME	0x0002
#define DUMP_NAME	0x0004
#define DUMP_FRACTION	0x0008
#define DUMP_RAW_RES	0x0010
#define DUMP_ID		0x0020
#define DUMP_TRAINEE	0x0100
#define DUMP_INSTRUC	0x0200
#define DUMP_ROOM	0x0400
#define DUMP_EQUIP	0x0800
#define DUMP_EVENT	0x1000
#define DUMP_CM_TP	0x2000
#define DUMP_TOTAL	0x4000
// exemple DUMP_EVENT|DUMP_TRAINEE|DUMP_INSTRUC|DUMP_ROOM|DUMP_TIME|DUMP_NAME|DUMP_WEEK
//	     1000	100		200	   400	    2		4	1      = 0x1707

// traduction numero de jour Unix en french (3 lettres) Dimanche = 0
const char * wday2fr( int wday );
// traduction unix time en texte dans un stream, avec separateur
void write_date( time_t it, FILE * fout, char sep );
void write_jdate( time_t it, FILE * fout, char sep );
void write_hour( time_t it, FILE * fout );
// nettoyage texte ADE : remplacer "??" par "e" etc...
int write_clean_name( const char * txt, FILE * fout );

enum CATEGORY { Trainee, Instructor, Room, Equipment };

class resource;
class table_resources;
class table_activities;
class table_events;
class jade_appli;

// les parametres de config de l'application JADE
class configuration {
public :
string xmlpath;			// chemin pour les fichier xml resultant de la connexion sur ADE
string xmlpath2;		// chemin pour les fichier xml secondaires pour comparaison
string preselpath;		// chemin pour liste de preselection pour multi dump
string difflistpath;		// chemin pour liste de ressources a differencier
string outpath;			// chemin pour fichiers de sortie t.q. dumps et diffs
vector <resource> subproj_spec;		// regexp et category des ressources du subproj
vector <resource> watched_spec;		// regexp et category des watched
// calendrier
int ade_epoch;	// unix time correspondant a getDate&week=0&day=0&slot=0, corrige de l'heure d'ete
		// c'est donc un lundi, à la premiere heure (7h45 par exemple)
unsigned int ade_week_0;	// semaine correspondant a cette epoch(32)
unsigned int ade_winter_week;	// semaine ADE du passage en heure d'hiver (typ.44-ade_week_0)
			// "Daylight saving " dernier dimanche de Mars -> dernier dimanche d'Octobre
unsigned int weeks_per_year;	// 52 ou 53
// vue graphique compacte
int qsemx;	// nombre de semaines par rangee
int qsemy;	// nombre de rangees de semaines
double daydx;	// pas horizontal des jours
double slotdy;	// hauteur d'un slot
int calsem0;	// premiere semaine
int calsemN;	// semaine de noel = premiere des vacances
int dump_button_flag;	// genre DUMP_EVENT|DUMP_TRAINEE|DUMP_INSTRUC|DUMP_ROOM|DUMP_TIME|DUMP_NAME|DUMP_WEEK

};

// une ressource
class resource {
public :
int ID;			// reverse ID
string name;		// ou regex si spec
string path;		// path dans l'arbre
CATEGORY category;
// constructeurs (le second sert pour construire une spec)
resource() : ID(-1), category(Trainee) {};
resource( CATEGORY cat, string pat ) : ID(-1), name(pat), category(cat) {};
// methods
const char * category2txt( CATEGORY c ) {
   switch( c ) {
	case Trainee :	  return "trainee"; break;
	case Instructor : return "instructor"; break;
	case Room :	  return "room"; break;
	case Equipment :  return "equipment"; break;
	}
   return "";
   };
int differenciator( jade_appli * ja, jade_appli * ja2, FILE * fil, int room_diff_flag );
void dump1( FILE * fil ) {
   fprintf( fil, "%4d %d %s %s%s\n", ID, (int)category, category2txt(category), path.c_str(), name.c_str() );
   }
};

// la table des ressources
class table_resources {
public :
vector<resource> table;		// toutes les activites
vector<int> ID2index;		// array donnant les index dans table en fonction de l'ID (-1 sinon)
vector<int> trindex;		// array donnant les id's dans l'ordre du tri
string noname;			// utile car echec frequent : nous ne savons pas traiter les folders de ressources ADE
// constructor
table_resources() : noname( string("no name") ) {
  ID2index.reserve( MAXID );
  ID2index.insert( ID2index.end(), MAXID, -1 );	// tableau pre-rempli de -1
  table.reserve( 1024 );				// juste pour gagner du temps
  trindex.reserve( 1024 );
  };
// accesseurs
resource * get_resource_by_ID( int id ) {
  if ( id < 0 ) return NULL;
  if ( ID2index.at(id) < 0 ) return NULL;
  return( &table.at(ID2index[id]) );
  };
resource * get_resource_by_spec( resource * spec );
string * get_resource_name( int id ) {
  resource * re = get_resource_by_ID( id );
  if	( re )
	return( &re->name );
  else	return( &noname );
  }
// methods
int load_xml( string fullpath );
void tri();
void dump( FILE * fil ) {
  fprintf( fil, "nombre de ressources %d=%d\n", (int)table.size(), (int)trindex.size() );
  unsigned int i;
  for	( i = 0; i < trindex.size(); ++i )
	get_resource_by_ID( trindex.at(i) )->dump1( fil );
  }
};

// "function object" pour comparer les activites selon ordre lexicografic
class res_compare {
  table_resources * tr;
public:
  // constructeur : pour initialiser tr
  res_compare(table_resources * t) : tr(t) {};
  // comparateur
  bool operator()( int i1, int i2 );
};

// une activite
class activity {
public :
int ID;			// reverse ID
string name;
string path;		// path dans l'arbre, vide si l'activite n'appartient pas au subpro
int durslots;		// duree en slots
int placedslots;	// duree placee en slots
set<int> resourceIDs;	// id's des resources imposees
set<int> resourceOrIDs;	// id's des resources requises
// methods
void dump1( FILE * fil );
void dump2( FILE * fil );
void dump3( FILE * fil, jade_appli *ja );
};

// la table des activites
class table_activities {
public :
vector<activity> table;		// toutes les activites
vector<int> ID2index;		// array donnant les index dans table en fonction de l'ID (-1 sinon)
vector<int> trindex;		// array donnant les id's dans l'ordre du tri, pour le sub-projet seulement
// constructor
table_activities() {
  ID2index.reserve( MAXID );
  ID2index.insert( ID2index.end(), MAXID, -1 );	// tableau pre-rempli de -1
  table.reserve( 256 );				// juste pour gagner du temps
  trindex.reserve( 256 );
  };
// accesseur
activity * get_activity_by_ID( int id ) {
  if ( ID2index.at(id) < 0 ) return NULL;
  return( &table.at(ID2index[id]) );
  };
// methods
int load_xml( string fullpath );
void tri();
void dump( FILE * fil, jade_appli *ja ) {
  fprintf( fil, "nombre d'activites %d=%d\n", (int)table.size(), (int)trindex.size() );
  unsigned int i;
  for	( i = 0; i < trindex.size(); ++i )
	get_activity_by_ID( trindex.at(i) )->dump3( fil, ja );
  }
};

// "function object" pour comparer les activites selon ordre lexicografic
class act_compare {
  table_activities * ta;
public:
  // constructeur : pour initialiser ta
  act_compare(table_activities * t) : ta(t) {};
  // comparateur
  bool operator()( int i1, int i2 );
};

class event {
public :
int ID;			// reverse ID
string name;		// seulement si l'event n'est pas dans notre BD
int activityID;		// id de l'activite (meme si elle n'existe pas dans notre BD)
int absslot;		// instant de debut en slots abs
int durslots;		// duree en slots
#ifdef ADE_COLOR
int color;		// couleur en RGB sur 24 bits
#endif
set<int> resourceIDs;	// id's des resources utilisees
// methodes
string * getname( jade_appli *ja );
bool intersect( int abs_slot ) {
  return ( ( abs_slot >= absslot ) && ( abs_slot < ( absslot + durslots ) ) );
  };
void gen_html_row( FILE * fil, jade_appli *ja, int ID, CATEGORY cat, bool reset_week, int i, int j );
void dump( FILE * fil, jade_appli *ja, int flag=(DUMP_TIME|DUMP_NAME), int i=0, int j=0 );
void door_dump( FILE * fil, jade_appli *ja );
};

// la table des events
class table_events {
public :
vector<event> table;		// tous les events
vector<int> ID2index;		// array donnant les index dans table en fonction de l'ID (-1 sinon)
vector<int> trindex;		// array donnant les id's dans l'ordre chronologique
// constructor
table_events() {
  ID2index.reserve( MAXID );
  ID2index.insert( ID2index.end(), MAXID, -1 );	// tableau pre-rempli de -1
  table.reserve( 256 );				// juste pour gagner du temps
  trindex.reserve( 256 );
  };
// accesseur
event * get_event_by_ID( int id ) {
  if ( ID2index.at(id) < 0 ) return NULL;
  return( &table.at(ID2index[id]) );
  };
// methods
int load_xml( string fullpath, jade_appli *ja, int preactflag );	// preact61 : activites doivent exister
void tri();
int next_event( int i, int resource_ID );
int next_event( int i, int resource_ID, vector<int> * subproj_res );
void dump( FILE * fil, jade_appli *ja, int flag=(DUMP_TIME|DUMP_NAME) ) {
  fprintf( fil, "nombre d'events %d=%d\n", (int)table.size(), (int)trindex.size() );
  unsigned int i;
  for	( i = 0; i < trindex.size(); ++i )
	get_event_by_ID( trindex.at(i) )->dump( fil, ja, flag );
  }
void gen_html_head_for_1_cat( FILE * fil, jade_appli *ja, string name, CATEGORY cat );
void gen_html_for_1_res( FILE * fil, jade_appli *ja, int ID );
void gen_html_file_for_1_res( const char * path, jade_appli *ja, int ID );
};

// "function object" pour comparer des events par ordre chronologique
class event_compare {
  table_events * te;
public:
  // constructeur : pour initialiser te
  event_compare(table_events * t) : te(t) {};
  // comparateur
  bool operator()( int i1, int i2 );
};

// selection d'events utilisant une ressource ET dont le nom matche la regexp
class selection {
public :
int ressourceID;	// ignore si -1
string regexp;		// ignoree si vide
vector<int> events;	// contenu de la selection (IDs)
int totalslots;		// duree totale, attendue ou effective
// constructeur
selection() : ressourceID(-1), totalslots(0) {};
selection( int ID, string re ) : ressourceID(ID), regexp(re) {};
selection( int ID, string re, int total ) : ressourceID(ID), regexp(re), totalslots(total) {};
// methodes
void clear() { events.clear(); }
int select( jade_appli *ja );		// effectuer la selection selon ressourceID et regexp
int find( int abs_slot, jade_appli *ja );	// trouver le premier event qui intersecte le slot
int dump( FILE * fil, jade_appli *ja, int flag=(DUMP_EVENT|DUMP_TIME|DUMP_NAME) );
void door_dump( FILE * fil, jade_appli *ja );
void gen_html( FILE * fil, jade_appli *ja );
};

class jade_appli {
public :
configuration config;
curlchunk lechunk;
table_resources tr;
table_activities ta;
table_events te;
map<int,int> used_resources;	// clef = id des ressources utilisees, valeur = (futur) bitmask
vector<int> subproject_res;	// ressources du sub project
vector<int> watched_res;	// ressources dont on surveille les events
vector<selection> selections;	// les selections
// methodes
int init_config( string configpath );

unsigned int ade2time( int absslot );	// traduction ADE absoluteSlot en unix time
unsigned int ade2week( int absslot );	// traduction ADE absoluteSlot en semaine calendaire
unsigned int week2ade( int week );	// traduction semaine calendaire en ADE absoluteSlot
unsigned int date2ade( int week, int day, int slot );	// traduction semaine calendaire, jour, slot en ADE absoluteSlot

int parse_events_for_res_list( vector<int> * list, string & xmlpath, int preactflag, int download_flag );
int ADE_session( int download_flag, int resource_only );
int local_session( int subproj_only );

int event_comparator( jade_appli * ja2, event * e1, event * e2 );
void dump_used_res( FILE * fil );
int dump_multi_select( FILE * fil, vector<selection> * liste, int flag=(DUMP_CM_TP|DUMP_EVENT|DUMP_TIME|DUMP_NAME) );
void gen_html_multi_select( FILE * fil, vector<selection> * liste );
};

// old fashioned html style
#define STYLE_BODY		"bgcolor=\"#ffffff\""
#define STYLE_TABLE		"cellpadding=\"2\" cellspacing=\"4\" bgcolor=\"#c0c0c0\" border=\"0\""
#define STYLE_TD_WEEK		"bgcolor=\"#90d0ff\""
#define STYLE_TD_DATE		"bgcolor=\"#ffffff\""
#define STYLE_TD_HOURS		"bgcolor=\"#ffffff\""
#define STYLE_TD_ROOM		"bgcolor=\"#ffffff\""
#define STYLE_TD_TITLE		"bgcolor=\"#ffffff\""
#define STYLE_TD_TRAINER	"bgcolor=\"#ffffff\""
#define STYLE_TD_TRAINEE	"bgcolor=\"#ffffff\""
#define STYLE_TD_HEAD_DATE	"bgcolor=\"#ffffff\""
#define STYLE_TD_HEAD_HOURS	"bgcolor=\"#ffffff\""
#define STYLE_TD_HEAD_ROOM	"bgcolor=\"#ffffff\""
#define STYLE_TD_HEAD_TITLE	"bgcolor=\"#ffffff\""
#define STYLE_TD_HEAD_TRAINER	"bgcolor=\"#ffffff\""
#define STYLE_TD_HEAD_TRAINEE	"bgcolor=\"#ffffff\""

