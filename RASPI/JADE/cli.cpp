#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

using namespace std;
#include <string>
#include <sstream>
#include <vector>
#include <set>

#include "curlchunk.h"
#include "xmlpc.h"
#include "jade.h"
#include "version.h"
#include "ssplit2.h"

FILE * logfil=stderr;

extern "C" {
/* gasp classique --> stderr */
void gasp( const char *fmt, ... )  /* fatal error handling */
{
  va_list  argptr;
  fprintf( logfil, "ERROR : " );
  va_start( argptr, fmt );
  vfprintf( logfil, fmt, argptr );
  va_end( argptr );
  fprintf( logfil, "\n" );
  fclose(logfil);
  exit(1);
}
}

/* ============================== operations optionnelles ====================== */

/* experience "multi_select_dump" pour stats, utilisant une liste de selection 
   fournie dans un fichier a part (pour le moment, trainees or instructor only)
   rend le nombre d'erreurs de total ou -1 si echec */
int multi_select_dump( jade_appli * ja, const char * fnam, int dump_flags  )
{
vector<selection> gross;
FILE * confil;
CATEGORY c1 = Trainee, c2 = Instructor;
resource zespec;
resource * zeres;
char fbuf[256]; int j;
vector <string> splut;

confil = fopen( ja->config.preselpath.c_str(), "r" );
if	( confil == NULL )
	gasp("Sorry, pas de fichier %s", ja->config.preselpath.c_str() );

while	( fgets( fbuf, sizeof(fbuf), confil ) )
	{
	if	( fbuf[0] == '#' )
		j = 0;
	else	j = ssplit2( &splut, string(fbuf) );
	if	( j > 1 )
		{
		zespec = resource( c1, splut[0] );
		zeres = ja->tr.get_resource_by_spec( & zespec );
		if	( zeres == NULL )
			{
			zespec = resource( c2, splut[0] );
			zeres = ja->tr.get_resource_by_spec( & zespec );
			if	( zeres == NULL )
				gasp("Sorry, pas de ressource pour %s", zespec.name.c_str() );
			}
		if	( j > 2 )
			{
			gross.push_back( selection( zeres->ID, splut[1], int( strtod( splut[2].c_str(), NULL ) / HOURperSLOT ) ) );
			}
		else	gross.push_back( selection( zeres->ID, splut[1] ) );
		}
	}
fclose( confil );

FILE * fil; int errcnt;

snprintf( fbuf, sizeof( fbuf ), "%s%s.txt", ja->config.outpath.c_str(), fnam );
fil = fopen( fbuf, "w");
if	( fil )
	{
	errcnt = ja->dump_multi_select( fil, &gross, dump_flags );
	fprintf( fil, "--> %d mismatches\n", errcnt );
	fclose( fil );
	return errcnt;
	}
else	return -1;
}

// experience muti-diff utilisant une liste de ressources fournie dans un fichier a part 
int multi_diff( jade_appli * ja, jade_appli * ja2 )
{
vector<resource> gross;
FILE * confil;
CATEGORY c;
resource zespec;
resource * zeres;
char fbuf[256]; int j;
vector <string> splut;
// etape 1 : lecture du fichier de config
confil = fopen( ja->config.difflistpath.c_str(), "r" );
if	( confil == NULL )
	gasp("Sorry, pas de fichier %s", ja->config.difflistpath.c_str() );

while	( fgets( fbuf, sizeof(fbuf), confil ) )
	{
	if	( fbuf[0] == '#' )
		j = 0;
	else	j = ssplit2( &splut, string(fbuf) );
	if	( j > 1 )
		{
		switch	( char(splut[0][0]) )
			{
			case 'T' : c = Trainee; break;
			case 'I' : c = Instructor; break;
			case 'R' : c = Room; break;
			default  : c = Equipment;
			}
		zespec = resource( c, splut[1] );
		zeres = ja->tr.get_resource_by_spec( & zespec );
		if	( zeres == NULL )
			gasp("Sorry, pas de ressource pour %s", zespec.name.c_str() );
		else	{
			gross.push_back( *zeres );
			}
		}
	}
fclose( confil );
// etape 2 : boucle sur les ressources
vector<resource>::iterator begin, end;
FILE * fil; int cnt;
begin = gross.begin();
end = gross.end();
while	( begin != end )
	{
	snprintf( fbuf, sizeof( fbuf ), "%sdiff_%d.txt", ja->config.outpath.c_str(), begin->ID );
	fil = fopen( fbuf, "w");
	if	( fil )
		{
		cnt = begin->differenciator( ja, ja2, fil, 1 );
		fclose( fil );
		fprintf( logfil, "%3d differences for res %4d : %s\n", cnt, begin->ID, begin->name.c_str() );
		if	( cnt == 0 )
			remove( fbuf );
		}
	++begin;
	}

return 0;
}


// generation de html personnalises pour tous les events d'1 Instructor
int single_dump( jade_appli * ja, const char * respat )
{
resource zespec;
resource * zeres;

zespec = resource( Instructor, string(respat) );
zeres = ja->tr.get_resource_by_spec( & zespec );
if	( zeres == NULL )
	gasp("Sorry, pas de ressource pour %s", zespec.name.c_str() );

ja->te.gen_html_file_for_1_res( NULL, ja, zeres->ID );
fprintf( logfil, "HTML generated for %s\n", zeres->name.c_str() );
return 0;
}


// generation de dump pour STM32, pour tous les events d'1 Room
int single_room_dump( jade_appli * ja, const char * respat )
{
resource zespec;
resource * zeres;
selection se;

// selectionner la room
zespec = resource( Room, string(respat) );	// respat c'est une regexp
zeres = ja->tr.get_resource_by_spec( & zespec );
if	( zeres == NULL )
	gasp("Sorry, pas de ressource pour %s", zespec.name.c_str() );
se = selection( zeres->ID, ".*" );		// le .* c'est une regexp pour selectionner des noms d'event
se.select( ja );

// ouverture du fichier
FILE * pfil; char fbuf[256];
snprintf( fbuf, sizeof( fbuf ), "%s%s.c", ja->config.outpath.c_str(), "zeroom" );

// creation du contenu
pfil = fopen( fbuf, "w");
if	( pfil )
	{
	se.door_dump( pfil, ja );
	fclose( pfil );
	}
fprintf( logfil, "door data generated for %s\n", zeres->name.c_str() );
return 0;
}


#ifdef WIN32
#define MYMKDIR(buf) mkdir(buf)
#else
#define MYMKDIR(buf) mkdir(buf,0755)
#endif

// produire un path en fonction de la date du jour pour cloud appli
// basepath0yy/mm/dd/hh_
// basepath doit finir par /, le path rendu non, car hh_ est incorpore au nom de fichier
// si necessaire, cree silencieusement les sous repertoires de maniere incrementale
string generate_date_path( string basepath )
{
struct tm *t; time_t curtime;
char tbuf[128]; int pos;
curtime = time (NULL);
t = localtime (&curtime);
pos = snprintf( tbuf, sizeof(tbuf), "%s", basepath.c_str() );
MYMKDIR( tbuf );
pos += snprintf( tbuf+pos, sizeof(tbuf)-pos, "0%02d/", t->tm_year - 100 );
MYMKDIR( tbuf );
pos += snprintf( tbuf+pos, sizeof(tbuf)-pos, "%02d/", t->tm_mon+1 );
MYMKDIR( tbuf );
pos += snprintf( tbuf+pos, sizeof(tbuf)-pos, "%02d/", t->tm_mday );
MYMKDIR( tbuf );
snprintf( tbuf+pos, sizeof(tbuf)-pos, "%02d_", t->tm_hour );
return string(tbuf);
}


/* ============================== the main ===================================== */
int main( int argc, char ** argv )
{
jade_appli jade;
jade_appli * ja2 = NULL;
int force_dl = 0, cloud_flag = 0;

int retval;
// ============================================================ config JADE
retval = jade.init_config( string("./jade.conf") );
if	( retval )
	gasp("failed init_config %d", retval );

if	( argc > 1 )
	{
	switch	( argv[1][1] )
		{
		case 'f' : force_dl = 1; break;
		case 'r' : force_dl = -1; break;
		// mode ping-pong (subdirs 0 et 1 de jade.config.xmlpath)
		// N.B. dans tous les cas xmlpath c'est l'actuel, xmlpath2 l'ancien
		case '1' :
			{
			jade.config.xmlpath2 = jade.config.xmlpath + string("0/");
			jade.config.xmlpath  = jade.config.xmlpath + string("1/");
			force_dl = 1; cloud_flag = 1;
			} break;
		case '0' :
			{
			jade.config.xmlpath2 = jade.config.xmlpath + string("1/");
			jade.config.xmlpath  = jade.config.xmlpath + string("0/");
			force_dl = 1; cloud_flag = 1;
			} break;
		}
	}

// mode cloud, active par les options -0 et -1 :
//	- outpath est augmente d'une hierarchie par dates
//	- les messages de log sont envoyes dans un fichier au lieu de stderr
if	( cloud_flag )
	{
	char tbuf[128];
	jade.config.outpath = generate_date_path( jade.config.outpath );
	snprintf( tbuf, sizeof(tbuf), "%saa_log.txt", jade.config.outpath.c_str() );
	logfil = fopen( tbuf, "a" );
	if	( logfil == NULL )
		logfil = stderr;
	}
else	MYMKDIR( jade.config.outpath.c_str() );

fprintf( logfil, "JLNs JADE V%d.%d%c [%s]\n", VERSION, SUBVERS, BETAVER, jade.config.xmlpath.c_str() );

// ============================================================== read old XML if needed 

// la presence du param XMLPATH2 dans jade.conf active la creation de ja2, qui va activer
// la detection des changements aka "multi_diff"
// qui necessite le param DIFFLISTPATH

if	( jade.config.xmlpath2.size() )
	{
	ja2 = new jade_appli( jade );		// construction par copie !
	retval = ja2->local_session( 1 );	// lecture des ressources et events
	if	( retval )
		gasp("local session on %s failed, code %d", jade.config.xmlpath2.c_str(), retval ); 
	/* all event dump *
	FILE * fil;
	fil = fopen("all_event_dump2.txt", "w");
	if	( fil )
		{
		ja2->te.dump( fil, ja2 );
		fclose( fil );
		}
	//*/
	}

// ============================================================== present XML, read or get 
retval = jade.ADE_session( force_dl, 0 );
if	( retval )
	gasp("ADE session on %s failed, code %d", jade.config.xmlpath.c_str(), retval ); 

// ========================================== a partir d'ici on a toutes les donnees en RAM

/* all event dump *
FILE * fil;
fil = fopen("all_event_dump.txt", "w");
if	( fil )
	{
	jade.te.dump( fil, &jade );
	fclose( fil );
	}
//*/
/* dump des ressources utilisees dans le subproject
   ne marche que si on a active preactflag dans l'appel a parse_events_for_res_list() *
fil = fopen("used_resource_dump.txt", "w");
if	( fil )
	{
	jade.dump_used_res( fil );
	fclose( fil );
	}
//*/


// ------------------------------------- HTML dumps
single_room_dump( &jade, "GEI 005" );

// ------------------------------------- multi-select check & dumps

// la presence du param PRESELPATH dans jade.conf active la production de un
// ou plusieurs fichiers multi_select_dump*.txt

if	( jade.config.preselpath.size() )
	{
	int errcnt; int flags;
	flags = DUMP_TOTAL;
	errcnt = multi_select_dump( &jade, "multi_select_check", flags );
	fprintf( logfil, "multi select check : %d discordances\n", errcnt );

	flags = DUMP_EVENT | DUMP_CM_TP | DUMP_TOTAL | DUMP_TIME | DUMP_NAME | DUMP_FRACTION;
	multi_select_dump( &jade, "multi_select_dump.txt", flags );

	if	( cloud_flag == 0 )
		{
		flags = DUMP_EVENT | DUMP_TOTAL | DUMP_TIME | DUMP_NAME | DUMP_TRAINEE	| DUMP_INSTRUC;
		multi_select_dump( &jade, "multi_select_dump_pe.txt", flags );
		flags = DUMP_EVENT | DUMP_TOTAL | DUMP_TIME | DUMP_NAME | DUMP_ROOM;
		multi_select_dump( &jade, "multi_select_dump_s.txt", flags );
		}
	/*
	fil = fopen("multi_select_dump.html", "w");
	if	( fil )
		{
		ja->gen_html_multi_select( fil, &gross );
		fclose( fil );
		}
	*/
	}


// ------------------------------------- multi-diff

if	( ja2 )
	{
	multi_diff( &jade, ja2 );
	delete ja2;
	}

fclose(logfil);
return 0;
}
