#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include <pcre.h>

using namespace std;
#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>

#include "curlchunk.h"
#include "xmlpc.h"
#include "pcreux.h"
#include "ssplit2.h"
#include "jade.h"

extern "C" {
void gasp( const char *fmt, ... );  /* fatal error handling */
}

// methodes

// differenciator : detection de changements d'evenements pour 1 ressource
// par comparaison avec une version anterieure
// N.B. la version "supposée anterieure" est celle qui porte le numero 2
// rend le nombre de differences
int resource::differenciator( jade_appli * ja, jade_appli * ja2, FILE * fil, int room_diff_flag )
{
table_events * te1, * te2; int diffcnt = 0;
te1 = &ja->te; te2 = &ja2->te;

fprintf( fil, "----------- ressource %d --> %s ----------------\n", ID, name.c_str() );

// on va parcourir les evenements dans l'ordre chronologique
// on ne considere que les evenements qui implique la ressource consideree,
// et au moins une ressource du sub-projet
int i1, i2; int flag;
event * e1, * e2;
int t1, t2, resu;
vector<int> * sub = &ja->subproject_res;
i1 = te1->next_event( -1, ID, sub );
i2 = te2->next_event( -1, ID, sub );

while	( ( i1 >= 0 ) || ( i2 >= 0 ) )
	{
	// ici tres smart on evite de repeter une ressource qu'on connait deja
	flag = DUMP_TIME | DUMP_NAME;
	if	( category == Trainee )
		flag |= DUMP_INSTRUC;
	else if	( category == Instructor )
		flag |= DUMP_TRAINEE;
	else	flag |= DUMP_INSTRUC | DUMP_TRAINEE;
	if	( i1 < 0 )
		{
		// report e2 as unmatched
		fprintf( fil, "SUPPRIME ================\n"); ++diffcnt;
		e2 = te2->get_event_by_ID( te2->trindex.at(i2) );
		e2->dump( fil, ja2, flag );
		i2 = te2->next_event( i2, ID, sub );
		continue;
		}
	if	( i2 < 0 )
		{
		// report e1 as unmatched
		fprintf( fil, "AJOUTE ==================\n"); ++diffcnt;
		e1 = te1->get_event_by_ID( te1->trindex.at(i1) );
		e1->dump( fil, ja, flag );
		i1 = te1->next_event( i1, ID, sub );
		continue;
		}
	e1 = te1->get_event_by_ID( te1->trindex.at(i1) );
	e2 = te2->get_event_by_ID( te2->trindex.at(i2) );
	t1 = e1->absslot;
	t2 = e2->absslot;
	if	( t1 < t2 )
		{
		// report e1 as unmatched
		fprintf( fil, "AJOUTE ==================\n"); ++diffcnt;
		e1->dump( fil, ja, flag );
		i1 = te1->next_event( i1, ID, sub );
		}
	else if ( t2 < t1 )
		{
		// report e2 as unmatched
		fprintf( fil, "SUPPRIME ================\n"); ++diffcnt;
		e2->dump( fil, ja2, flag );
		i2 = te2->next_event( i2, ID, sub );
		}
	else	{
		if	( ( resu = ja->event_comparator( ja2, e1, e2 ) ) )
			{
			if	( resu & 1 )
				fprintf( fil, "MODIF heure =============\n");
			else if	( resu & 4 )
				fprintf( fil, "MODIF intitule ==========\n");
			else if	( resu & 2 )
				fprintf( fil, "MODIF duree =============\n");
			else if	( ( resu & 0x330 ) || ( ( resu & 0x440 ) && ( room_diff_flag ) ) )
				{
				fprintf( fil, "MODIF ressources ======== ");
				if	( resu & 0x100 ) { fprintf( fil, "-groupe " ); flag |= DUMP_TRAINEE; }
				if	( resu & 0x010 ) { fprintf( fil, "+groupe " ); flag |= DUMP_TRAINEE; }
				if	( resu & 0x200 ) { fprintf( fil, "-prof " ); flag |= DUMP_INSTRUC; }
				if	( resu & 0x020 ) { fprintf( fil, "+prof " ); flag |= DUMP_INSTRUC; }
				if	( resu & 0x400 ) { fprintf( fil, "-salle " ); flag |= DUMP_ROOM; }
				if	( resu & 0x040 ) { fprintf( fil, "+salle " ); flag |= DUMP_ROOM; }
				fprintf( fil, "\n");
				}
			else	flag = 0;
			if	( flag )
				{
				++diffcnt; e2->dump( fil, ja2, flag );
				fprintf( fil, "DEVIENT =================\n");
				++diffcnt; e1->dump( fil, ja, flag );
				}
			}
		i1 = te1->next_event( i1, ID, sub );
		i2 = te2->next_event( i2, ID, sub );
		}
	}
fprintf( fil, "----------- ressource %d --> %d differences -------------\n",
	 ID, diffcnt );

return diffcnt;
}

int table_resources::load_xml( string fullpath )
{
xmlobj * lexml = new xmlobj( fullpath.c_str(), NULL );
if	( !(lexml->is) )
	return 1;
int status;
// valeurs temporaires
xelem * elem, * elem2; resource * r; int i, id; string s;

while	( ( status = lexml->step() ) )
	{
	elem = &lexml->stac.back();
	switch	( status )
		{
		case 1 :
		// printf("~~~> %s\n", elem->tag.c_str() );
		if	( ( elem->tag == string("leaf") ) || ( elem->tag == string("branch") ) )
			{
			i = table.size();
			table.push_back( resource() );		// creer la resource puis l'indexer
			r = &table.back();
			s = elem->attr[string("id")];
			id = atoi( s.c_str() );
			r->ID = id;
			ID2index.at(id) = i;
			trindex.push_back(id);
			s = elem->attr[string("name")];		// copier son nom
			if	( s.size() == 0 )
				s = string("noname");
			r->name = s;
			int i;		// son path : il faut derouler le stac
					// meme si on n'a pas besoin de path, on doit derouler pour category
			for	( i = 1; i < (int)lexml->stac.size() - 1; ++i )
				{
				elem2 = &lexml->stac.at(i);
				if	( elem2->tag == string("category") )
					{
					s = elem2->attr[string("category")];
					if	( s == string("trainee") )	r->category = Trainee;
					else if	( s == string("instructor") )	r->category = Instructor;
					else if	( s == string("classroom") )	r->category = Room;
					else					r->category = Equipment;
					}
				else	{
					r->path += elem2->attr[string("name")];
					r->path += string("|");
					}
				}
			}
		break;
		case 2 :
		break;
		default :
		gasp("%s ligne %d : syntaxe xml %d", lexml->filepath, (lexml->curlin+1), status );
		}
	}	// while status
return 0;
}

// fonction pour le tri des resources par ordre lexicographique
bool res_compare::operator()( int id1, int id2 )
{
resource * r1 = tr->get_resource_by_ID( id1 );
resource * r2 = tr->get_resource_by_ID( id2 );
if	( r1->category == r2->category )
	{
	if	( r1->path == r2->path )
		return( r1->name < r2->name );
	else	return( r1->path < r2->path );
	}
else	return( r1->category < r2->category );
}

void table_resources::tri()
{
sort( trindex.begin(), trindex.end(), res_compare( this ) );
}

// rend NULL si pas de match, retourne au premier match
resource * table_resources::get_resource_by_spec( resource * spec )
{
pcreux * lareu;
if	( spec->name.size() )
	lareu = new pcreux( spec->name.c_str() );
else	return NULL;
resource * resou = NULL;
unsigned int i; string * lenom;
for	( i = 0; i < table.size(); ++i )
	{
	lenom = &table[i].name;
	//printf("? %s %d\n", lenom->c_str(), (int)table[i].category );
	if	( table[i].category == spec->category )
		{
		lenom = &table[i].name;
		//printf("? %s %d\n", lenom->c_str(), (int)table[i].category );
		if	( lareu->matchav( lenom->c_str(), lenom->size() ) > 0 )
			{
			resou = &table[i];
			break;
			}
		}
	}
if	( lareu )
	delete( lareu );
return resou;
}

int table_activities::load_xml( string fullpath )
{
xmlobj * lexml = new xmlobj( fullpath.c_str(), NULL );
if	( !(lexml->is) )
	return 1;
int status;
// valeurs temporaires
xelem * elem; activity * a; int i, id; string s;
int orflag = 0;

while	( ( status = lexml->step() ) )
	{
	elem = &lexml->stac.back();
	switch	( status )
		{
		case 1 :
		// printf("~~~> %s\n", elem->tag.c_str() );
		if	( elem->tag == string("activity") )
			{
			i = table.size();
			table.push_back( activity() );		// creer l'activite puis l'indexer
			a = &table.back();
			s = elem->attr[string("id")];
			id = atoi( s.c_str() );
			a->ID = id;
			ID2index.at(id) = i;
			trindex.push_back(id);
			s = elem->attr[string("name")];		// copier son nom
			if	( s.size() == 0 )
				s = string("noname");
			a->name = s;
			s = elem->attr[string("duration")];	// copier sa duree nominale
			i = atoi( s.c_str() );
			a->durslots = i / MNperSLOT;
			s = elem->attr[string("durationInMinutes")];	// sa duree placee
			i = atoi( s.c_str() );
			a->placedslots = i / MNperSLOT;
			int i;				// son path : il faut derouler le stac
			for	( i = 1; i < (int)lexml->stac.size() - 1; ++i )
				{
				a->path += lexml->stac.at(i).attr[string("name")];
				a->path += string("|");
				}
			}
		if	( elem->tag == string("orRequest") )
			orflag = 1;
		if	( elem->tag == string("and") )		// integrer une ressource
			{
			s = elem->attr[string("id")];
			i = atoi( s.c_str() );
			if	( orflag )
				table.back().resourceOrIDs.insert(i);
			else	table.back().resourceIDs.insert(i);
			}
		break;
		case 2 :
		if	( elem->tag == string("orRequest") )
			orflag = 0;
		break;
		default :
		gasp("%s ligne %d : syntaxe xml %d", lexml->filepath, (lexml->curlin+1), status );
		}
	}	// while status
return 0;
}

// fonction pour le tri des activites par ordre lexicographique
bool act_compare::operator()( int id1, int id2 )
{
activity * a1 = ta->get_activity_by_ID( id1 );	// &(ta->table.at(ta->ID2index.at(id1)));
activity * a2 = ta->get_activity_by_ID( id2 );
if	( a1->path == a2->path )
	return( a1->name < a2->name );
else	return( a1->path < a2->path );
}

void table_activities::tri()
{
sort( trindex.begin(), trindex.end(), act_compare( this ) );
}

string * event::getname( jade_appli *ja )
{
activity * act = ja->ta.get_activity_by_ID(activityID);
if	( act )
	return &act->name;
else	return &name;
}

#ifdef ADE_COLOR
// traduction couleur ADE en 24 bits
static int a2color( const char * txt )
{
int resu; int RGB[3];
resu = sscanf( txt, "%d,%d,%d", &RGB[0], &RGB[1], &RGB[2] );
if	( resu != 3 )
	return 0;
resu  =   RGB[2] & 0xFF;
resu |= ( RGB[1] & 0xFF ) << 8;
resu |= ( RGB[0] & 0xFF ) << 16;
return resu;
}
#endif

// si preactflag, c'est pour les events des resources (groupes d'eleves) de notre sub-project :
//	- activite non existante donne erreur
//	- ressources utilisees par events sont ajoutees a ja->used_resources[]
// sinon c'est pour les events des autres ressource, dont on veut observer le planning sans plus
int table_events::load_xml( string fullpath, jade_appli *ja, int preactflag )
{
activity * act;
xmlobj * lexml = new xmlobj( fullpath.c_str(), NULL );
if	( !(lexml->is) )
	return 1;
int status;
// valeurs temporaires
xelem * elem; event * e; int i, id; string s;
int eventisnew = 0;

while	( ( status = lexml->step() ) )
	{
	elem = &lexml->stac.back();
	switch	( status )
		{
		case 1 :
		// printf("~~~> %s\n", elem->tag.c_str() );
		if	( elem->tag == string("event") )
			{
			s = elem->attr[string("id")];
			id = atoi( s.c_str() );
			if	( get_event_by_ID( id ) == NULL )
				{
				i = table.size();
				table.push_back( event() );		// creer l'event puis l'indexer
				e = &table.back();
				e->ID = id;
				ID2index.at(id) = i;
				trindex.push_back(id);

				s = elem->attr[string("absoluteSlot")];	// copier sa date
				i = atoi( s.c_str() );
				e->absslot = i;

				eventisnew = 1;				// autoriser lecture resources

				s = elem->attr[string("activityId")];	// copier l'id de l'activite
				i = atoi( s.c_str() );
				e->activityID = i;
				act = ja->ta.get_activity_by_ID(i);

				s = elem->attr[string("duration")];	// copier sa duree nominale
				i = atoi( s.c_str() );
				e->durslots = i;

				s = elem->attr[string("name")];		// extraire son nom
				if	( s.size() == 0 )
					s = string("noname");

				if	( act )				// ici tester act
					{
					if	( act->name != s )
						gasp("activity %d name != event %d name", e->activityID, e->ID );
					if	( act->durslots != e->durslots )
						gasp("activity %d duration != event %d duration", e->activityID, e->ID );
					}
				else	{
					if	( preactflag )
						gasp("missing activity %d for event %d", e->activityID, e->ID );
					e->name = s;			// copier nom
					}
				#ifdef ADE_COLOR
				s = elem->attr[string("color")];	// copier sa duree nominale
				i = a2color( s.c_str() );
				e->color = i;
				#endif
				}
			}
		if	( elem->tag == string("resource") )	// integrer une ressource
			{
			if	( eventisnew )
				{
				s = elem->attr[string("id")];
				i = atoi( s.c_str() );
				table.back().resourceIDs.insert(i);
				if	( preactflag )
					ja->used_resources[i] = 0;	// ajouter au catalogue si elle n'y est pas
				}
			}
		break;
		case 2 :
		if	( elem->tag == string("event") )
			eventisnew = 0;
		break;
		default :
		gasp("%s ligne %d : syntaxe xml %d", lexml->filepath, (lexml->curlin+1), status );
		}
	}	// while status
return 0;
}

// fonction pour le tri des events par ordre chronologique
bool event_compare::operator()( int id1, int id2 )
{
event * e1 = te->get_event_by_ID( id1 );	// &(ta->table.at(ta->ID2index.at(id1)));
event * e2 = te->get_event_by_ID( id2 );
return( e1->absslot < e2->absslot );
}

void table_events::tri()
{
sort( trindex.begin(), trindex.end(), event_compare( this ) );
}

// chercher le prochain event concernant la ressource
// donner -1 pour trouver le premier
// rend -1 s'il n'y a plus d'evenement
int table_events::next_event( int i, int resource_ID )
{
event * eve;
++i;
while	( i < (int)trindex.size() )
	{
	eve = get_event_by_ID( trindex.at(i) );
	if	( eve->resourceIDs.count( resource_ID ) )
		return i;
	++i;
	}
return -1;
}

// chercher le prochain event concernant la ressource resource_ID
// et au moins une ressource de la liste subproj_res
// donner -1 pour trouver le premier
// rend -1 s'il n'y a plus d'evenement
int table_events::next_event( int i, int resource_ID, vector<int> * subproj_res )
{
event * eve;
vector<int>::iterator ibeg, iend;
iend = subproj_res->end();
++i;
while	( i < (int)trindex.size() )
	{
	eve = get_event_by_ID( trindex.at(i) );
	if	( eve->resourceIDs.count( resource_ID ) )
		{				// on a pense a utilise l'algo d'intersection de sets
		ibeg = subproj_res->begin();	// mais il est lourd a mettre en oeuvre et non-optimal
		while	( ibeg != iend )	// car nous ont peut s'arreter des que l'intersection a
			{			// juste un element
			if	(  eve->resourceIDs.count( *(ibeg++) ) )
				return i;
			}
		}
	++i;
	}
return -1;
}

// effectuer la selection selon ressourceID et regexp
int selection::select( jade_appli *ja )
{
pcreux * lareu;
if	( regexp.size() )
	lareu = new pcreux( regexp.c_str() );
else	lareu = NULL;
unsigned int i; int eid; event * eve; string * lenom;
for	( i = 0; i < ja->te.trindex.size(); ++i )
	{
	eid = ja->te.trindex.at(i);
	eve = ja->te.get_event_by_ID( eid );
	// critere ressourceID eventuellement bypasse par -1
	if	( ( ressourceID == -1 ) || ( eve->resourceIDs.count( ressourceID ) ) )
		{
		// critere regexp eventuellement bypasse par ""
		if	( lareu )
			{
			lenom = eve->getname( ja );
			if	( lareu->matchav( lenom->c_str(), lenom->size() ) > 0 )
				events.push_back( eid );
			}
		else	events.push_back( eid );
		}
	}
if	( lareu )
	delete( lareu );
return events.size();
}

// trouver le premier event qui intersecte le slot, rendre ID ou -1
int selection::find( int abs_slot, jade_appli *ja )
{
unsigned int i; int ID; event * eve;
for	( i = 0; i < events.size(); ++i )
	{
	ID = events[i];
	eve = ja->te.get_event_by_ID( ID );
	if	( eve )
		{
		if	( eve->intersect( abs_slot ) )
			return ID;
		}
	}
return -1;
}

// comparaison de 2 event, la valeur de retour indique en quoi ils different
// zero si identiques
int jade_appli::event_comparator( jade_appli * ja2, event * e1, event * e2 )
{
if	( e1->absslot != e2->absslot )
	return 1;
if	( ( *e1->getname( this ) ) != ( *e2->getname( ja2 ) ) )
	return 4;
if	( e1->durslots != e2->durslots )
	return 2;
if	( e1->resourceIDs != e2->resourceIDs )		// comparaison de sets d'un coup !!!
	{
	int retval = 0;
	resource * reso; 			// Cherchons ressource qui soit dans e1 mais pas e2
	set<int>::iterator begin=e1->resourceIDs.begin(), end=e1->resourceIDs.end();
	while	( begin != end )
		{
		if	( e2->resourceIDs.count( *begin ) == 0 )
			{
			reso = this->tr.get_resource_by_ID( *begin );
			if	( reso )
				switch	( reso->category )
					{
					case Trainee :	  retval |= 0x10; break;
					case Instructor : retval |= 0x20; break;
					case Room :	  retval |= 0x40; break;
					default :	  retval |= 0x80; break;
					}
			}
		++begin;
		} 				// Cherchons ressource qui soit dans e2 mais pas e1
	begin=e2->resourceIDs.begin(); end=e2->resourceIDs.end();
	while	( begin != end )
		{
		if	( e1->resourceIDs.count( *begin ) == 0 )
			{
			reso = ja2->tr.get_resource_by_ID( *begin );
			if	( reso )
				switch	( reso->category )
					{
					case Trainee :	  retval |= 0x100; break;
					case Instructor : retval |= 0x200; break;
					case Room :	  retval |= 0x400; break;
					default :	  retval |= 0x800; break;
					}
			}
		++begin;
		}
	return retval;
	}
return 0;
}

//* ============================== THE TIME ==================================== **/

// traduction ADE absoluteSlot en unix time
// attention absslot est base sur le localtime, la conversion change donc selon la saison !
unsigned int jade_appli::ade2time( int absslot )
{
unsigned int absday = absslot / SLOTperDAY;
unsigned int slot = absslot % SLOTperDAY;
unsigned int time = config.ade_epoch + ( absday * SECperDAY ) + ( slot * SECperSLOT );
// passage a l'heure d'hiver = ajouter 1 heure car localtime en enlevera une
unsigned int absweek = absday / 7;
if	( absweek >= config.ade_winter_week )
	time += 3600;
return time;
}

// traduction ADE absoluteSlot en semaine calendaire
unsigned int jade_appli::ade2week( int absslot )
{
unsigned int week = absslot / (SLOTperDAY*7) + config.ade_week_0;
if	( week > config.weeks_per_year )
	week -= config.weeks_per_year;
return week;
}

// traduction semaine calendaire en ADE absoluteSlot
unsigned int jade_appli::week2ade( int week )
{
if	( week < (int)config.ade_week_0 )
	week += config.weeks_per_year;
return( ( week - config.ade_week_0 ) * (SLOTperDAY*7) );
}

// traduction semaine calendaire, jour, slot en ADE absoluteSlot (Lundi = 0)
unsigned int jade_appli::date2ade( int week, int day, int slot )
{
unsigned int absslot = week2ade( week );
absslot += ( SLOTperDAY * day );
absslot += slot;
return absslot;
}

// traduction numero de jour Unix en french (3 lettres)
// Dimanche = 0
const char * wday2fr( int wday )
{
const char * j;
switch	( wday )
	{
	case 1 : j = "Lun"; break;
	case 2 : j = "Mar"; break;
	case 3 : j = "Mer"; break;
	case 4 : j = "Jeu"; break;
	case 5 : j = "Ven"; break;
	case 6 : j = "Sam"; break;
	default : j = "Dim";
	}
return j;
}

// traduction unix time en texte dans un stream, avec separateur
void write_date( time_t it, FILE * fout, char sep )
{
struct tm *t;
t = localtime( &it );
fprintf( fout, "%02d%c%02d%c%4d", t->tm_mday, sep, t->tm_mon+1, sep, t->tm_year+1900 );
}

// traduction unix time en texte dans un stream avec jour en french, avec separateur
void write_jdate( time_t it, FILE * fout, char sep )
{
struct tm *t;
t = localtime( &it );
fprintf( fout, "%s %02d%c%02d%c%4d", wday2fr( t->tm_wday ),
	 t->tm_mday, sep, t->tm_mon+1, sep, t->tm_year+1900 );
}

// traduction unix time en texte dans un stream
void write_hour( time_t it, FILE * fout )
{
struct tm *t;
// printf("sizeof(time_t) = %d\n", (int)sizeof(time_t) );
t = localtime( &it );
fprintf( fout, "%02dh%02d", t->tm_hour, t->tm_min );
}

/*
// conversion 3 entiers naifs en unix time
int mk_date( int d, int m, int y )
// int dmy2u( int d, int m, int y )
{
struct tm t;
if (
   ( d < 1 ) || ( d > 31 ) ||
   ( m < 1 ) || ( m > 12 ) ||
   ( y < 2000 ) || ( y > 2060 )
   ) return(-1);
t.tm_year  = y - 1900;
t.tm_mon   = m - 1;	// months since January [0,11]
t.tm_mday  = d;		// day of the month     [1,31]
t.tm_hour  = 0;
t.tm_min   = 0;
t.tm_sec   = 0;
t.tm_isdst = -1;
return( (int) mktime( &t ) );
}
*/

// nettoyage texte ADE : remplacer "??" par "e", &amp; par "&", &apos; par "'"
// rend le nombre de chars, supporte fout == NULL
int write_clean_name( const char * txt, FILE * fout )
{
int c, cnt=0, qcnt=0, ecnt=0;
while	( ( c = *(txt++) ) )
	{
	if	( c == '?' )
		{
		if	( ++qcnt >= 2 )
			{ ++cnt; if ( fout ) fprintf( fout, "e" ); }
		}
	else	{
		qcnt = 0;
		if	( c == '&' )
			ecnt = 1;
		else if	( ecnt )
			{
			++ecnt;
			if	( c == ';' )
				{
				++cnt;
				if	( fout )
					{
					if	( strncmp( txt-ecnt+1, "amp", 3 ) == 0 )
						fprintf( fout, "&" );
					else if	( strncmp( txt-ecnt+1, "apos", 4 ) == 0 )
						fprintf( fout, "'" );
					else	fprintf( fout, " " );
					}
				ecnt = 0;
				}
			}
		else	{ ++cnt; if ( fout ) fprintf( fout, "%c", c ); }
		}
	}
return cnt;
}

//* ============================== THE CONFIG ENGINE =================================== **/
/* la config se fait en 3 etapes :
	1) lecture d'un fichier texte contenant dans chaque ligne un couples nom - valeur
	   et garnissage d'une map temporaire de strings
	2) renseignement des membres scalaires de jade_appli::config et jade_appli::lechunk au moyen de cette map
	3) renseignement des tableaux de jade_appli::config au moyen de fichiers dont les paths
	   sont parmi les scalaires
 */

int jade_appli::init_config( string configpath )
{
FILE * confil;
char fbuf[256]; int j;
map<string, string> dico;
vector <string> splut;
// etape 1
confil = fopen( configpath.c_str(), "r" );
if	( confil == NULL )
	return( 101 );
while	( fgets( fbuf, sizeof(fbuf), confil ) )
	{
	if	( fbuf[0] == '#' )
		j = 0;
	else	j = ssplit2( &splut, string(fbuf) );
	if	( j > 1 )
		dico[ splut[0] ] = splut[1];
	}
fclose( confil );
/* dump pour debug *
map<string, string>::iterator itou = dico.begin();
while	( itou != dico.end() )
	{
	printf("|%s| --> |%s|\n", itou->first.c_str(), itou->second.c_str() );
	++itou;
	}
//*/
// etape 2
config.xmlpath  = dico[string("XMLPATH")];
config.xmlpath2 = dico[string("XMLPATH2")];
// config curlchunk
lechunk.ade_host  = dico[string("ADE_HOST")];
lechunk.ade_path  = dico[string("ADE_PATH")];
lechunk.login_pwd = dico[string("LOGIN_PWD")];
lechunk.projectid = dico[string("PROJECTID")];
// config calendrier
config.ade_epoch = atoi( dico[string("ADE_EPOCH")].c_str() );		// slot zero d'ADE en temps Unix
config.ade_week_0 = atoi( dico[string("ADE_WEEK_0")].c_str() );		// semaine correspondant a cette epoch
config.ade_winter_week = atoi( dico[string("CAL_WINTER_WEEK")].c_str() ) - config.ade_week_0;	// semaine ADE du passage en heure d'hiver
config.weeks_per_year = atoi( dico[string("WEEKS_PER_YEAR")].c_str() );	// semaines par an (52 ou 53)
// config geometrie
if	( dico.count(string("CALSEM0")) )
	{
	config.calsem0 = atoi( dico[string("CALSEM0")].c_str() );
	config.calsemN = atoi( dico[string("CALSEMN")].c_str() );
	config.qsemx =   atoi( dico[string("QSEMX")].c_str() );			// classic 5, wide 3
	config.qsemy =   atoi( dico[string("QSEMY")].c_str() );			// classic 3
	config.daydx =   (double)atoi( dico[string("DAYDX")].c_str() );		// classic 45  min 36 wide 75
	config.slotdy =  (double)atoi( dico[string("SLOTDY")].c_str() );	// classic 4
	config.dump_button_flag = strtol( dico[string("DUMP_BUT_FLAG")].c_str(), NULL, 0 );	// 0x accepted
	// classic flag : DUMP_EVENT|DUMP_TRAINEE|DUMP_INSTRUC|DUMP_ROOM|DUMP_TIME|DUMP_NAME|DUMP_WEEK;
	printf("but flag = %04x\n", config.dump_button_flag );
	}
config.preselpath = dico[string("PRESELPATH")];
config.difflistpath = dico[string("DIFFLISTPATH")];
config.outpath = dico[string("OUTPATH")];
// etape 3
CATEGORY c;
vector <resource> * pspec;
// fichier sub-project : un couple categorie - regexp par ligne
confil = fopen( dico[string("SUBPROJPATH")].c_str(), "r" );
if	( confil == NULL )
	return( 102 );
pspec = &config.subproj_spec;
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
		pspec->push_back( resource( c, splut[1] ) );
		}
	}
fclose( confil );
// fichier watched resources : un couple categorie - regexp par ligne
confil = fopen( dico[string("WATCHEDPATH")].c_str(), "r" );
if	( confil == NULL )
	return( 103 );
pspec = &config.watched_spec;
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
		pspec->push_back( resource( c, splut[1] ) );
		}
	}
fclose( confil );
return 0;
}

//* ============================== THE INPUT ENGINE ==================================== **/


int jade_appli::parse_events_for_res_list( vector<int> * list, string & xmlpath, int preactflag, int download_flag )
{
vector<int>::iterator ibeg = list->begin();
vector<int>::iterator iend = list->end();
ostringstream os; string funcode, fnam; int retval;
// char fnam[128];
while	( ibeg != iend )
	{
	os.str( string("") );	// vidage du stream !
	os << "getEvents&resources=" << *(ibeg++) << "&detail=8";
	funcode = os.str();
	fnam = lechunk.grab_xml( xmlpath, funcode, download_flag );
	retval = te.load_xml( fnam, this, preactflag );
	if	( retval )
		return retval;
	}
return 0;
}

// session de parsage xml avec download eventuel, a la demande ou forces
// attention : peupler d'abord config.subproj_spec[] et config.watched_spec[]
// pour faciliter cela on peut d'abord faire ADE_session avec resource_only
int jade_appli::ADE_session( int download_flag, int resource_only )
{
string funcode;		// un morceau de la requete pour ADE, i.e. ce qui suit "function="
int retval;
string fnam;

/* affichage de la date de base pour verif */
printf("date de reference \"ade_epoch\" : s%d <==> %d\n", config.ade_week_0, config.ade_epoch );
write_jdate( config.ade_epoch, stdout, '-' );	printf("  ");
write_hour( config.ade_epoch, stdout );	printf("\n"); fflush(stdout);

// recuperation des ressources (toutes)
funcode = string("getResources&tree=true&detail=2");
fnam = lechunk.grab_xml( config.xmlpath, funcode, download_flag );
retval = tr.load_xml( fnam );
if	( retval ) return retval;

tr.tri();			// tri de toutes les ressources

if	( resource_only )
	{
	FILE * fil = fopen("all_resource_dump", "w");
	if	( fil )
		{
		tr.dump( fil );
		fclose( fil );
		}
	lechunk.closesession();
	return 0;
	}

// peupler subproject_res[] et watched_res[] a partir des regexes et categories prises
// dans config.subproj_spec[] et config.watched_spec[]
unsigned int i; resource * spec; resource * resou;
printf("subproject resources :\n"); fflush(stdout);
for	( i = 0; i < config.subproj_spec.size(); ++i )
	{
	spec = &config.subproj_spec[i];
	if	( ( resou = tr.get_resource_by_spec( spec ) ) )
		{
		spec->ID = resou->ID;
		printf("  %6d %32s --> %-32s\n", spec->ID, spec->name.c_str(), resou->name.c_str() ); fflush(stdout);
		subproject_res.push_back( resou->ID );
		}
	else	printf("OOPS %s not found\n", spec->name.c_str() ); fflush(stdout);
	}
printf("watched resources :\n"); fflush(stdout);
for	( i = 0; i < config.watched_spec.size(); ++i )
	{
	spec = &config.watched_spec[i];
	if	( ( resou = tr.get_resource_by_spec( spec ) ) )
		{
		spec->ID = resou->ID;
		printf("  %6d %32s --> %-32s\n", spec->ID, spec->name.c_str(), resou->name.c_str() ); fflush(stdout);
		watched_res.push_back( resou->ID );
		}
	else	printf("OOPS %s not found\n", spec->name.c_str() ); fflush(stdout);
	}
//*/

/* les activities de notre sub-project - cette etape est facultative *
funcode = string("getActivities&tree=true&resources=207&detail=15");
fnam = lechunk.grab_xml( config.xmlpath, funcode, download_flag );
retval = ta.load_xml( fnam );
if	( retval ) return retval;
ta.tri();
// ta.dump( stdout, this );
//*/

// preactflag = 1 n'est bon que si on a les activities...
// retval = parse_events_for_res_list( &subproject_res, 1, download_flag );

// parsage des events trouves dans les xml du subprojet, avec download eventuel, a la demande ou forces
retval = parse_events_for_res_list( &subproject_res, config.xmlpath, 0, download_flag );
if	( retval ) return retval;

// parsage des events trouves dans les xml des watched resources, avec download eventuel, a la demande ou forces
retval = parse_events_for_res_list( &watched_res, config.xmlpath, 0, download_flag );
if	( retval ) return retval;

lechunk.closesession();

watched_res.push_back( MAXID - 1 );

te.tri();
printf("fini tri de %d events\n", (int)te.trindex.size() ); fflush(stdout);

return 0;
}

// session de parsage xml sur fichiers locaux (path config.xmlpath2)
// attention : peupler d'abord config.subproj_spec[] et eventuellement config.watched_spec[]
int jade_appli::local_session( int subproj_only )
{
string funcode;		// un morceau de la requete pour ADE, i.e. ce qui suit "function="
int retval;
string fnam;
int download_flag = -1;

// recuperation des ressources (toutes)
funcode = string("getResources&tree=true&detail=2");
fnam = lechunk.grab_xml( config.xmlpath2, funcode, download_flag );
retval = tr.load_xml( fnam );
if	( retval ) return retval;
tr.tri();			// tri de toutes les ressources

// peupler subproject_res[] et watched_res[] a partir des regexes et categories prises
// dans config.subproj_spec[] et config.watched_spec[]
unsigned int i; resource * spec; resource * resou;
printf("subproject resources :\n");
for	( i = 0; i < config.subproj_spec.size(); ++i )
	{
	spec = &config.subproj_spec[i];
	if	( ( resou = tr.get_resource_by_spec( spec ) ) )
		{
		spec->ID = resou->ID;
		printf("  %6d %32s --> %-32s\n", spec->ID, spec->name.c_str(), resou->name.c_str() );
		subproject_res.push_back( resou->ID );
		}
	else	printf("OOPS %s not found\n", spec->name.c_str() );
	}
// parsage des events trouves dans les xml du subprojet
retval = parse_events_for_res_list( &subproject_res, config.xmlpath2, 0, download_flag );
if	( retval ) return retval;

if	( subproj_only == 0 )
	{
	printf("watched resources :\n");
	for	( i = 0; i < config.watched_spec.size(); ++i )
		{
		spec = &config.watched_spec[i];
		if	( ( resou = tr.get_resource_by_spec( spec ) ) )
			{
			spec->ID = resou->ID;
			printf("  %6d %32s --> %-32s\n", spec->ID, spec->name.c_str(), resou->name.c_str() );
			watched_res.push_back( resou->ID );
			}
		else	printf("OOPS %s not found\n", spec->name.c_str() );
		}
	// parsage des events trouves dans les xml des watched resources
	retval = parse_events_for_res_list( &watched_res, config.xmlpath2, 0, download_flag );
	if	( retval ) return retval;
	}

te.tri();
printf("fini tri de %d events\n", (int)te.trindex.size() );

return 0;
}

//* ============================== THE OUTPUT ENGINE ==================================== **/

// generation d'une rangee de table html, insertion d'une rangee de numero de semaine si nouvelle semaine
void event::gen_html_row( FILE * fil, jade_appli *ja, int ID, CATEGORY cat, bool reset_week, int i, int j )
{
// insertion ligne si semaine nouvelle
static int oldweek = -1;
int qcol = 5;
if	( ( cat != Instructor ) && ( cat != Trainee ) )
	qcol = 6;
if	( reset_week )
	oldweek = -1;
int week = ja->ade2week( absslot );
if	( week != oldweek )
	{
	oldweek = week;
	fprintf( fil, "<tr><td colspan=\"%d\" %s>Week %d</td></tr>\n", qcol, STYLE_TD_WEEK, week );
	}
int rid; resource * re; const char * rname;
// la date et l'heure
unsigned int t = ja->ade2time( absslot );
fprintf( fil, "<tr><td %s>", STYLE_TD_DATE ); write_jdate( t, fil, '/' ); fprintf( fil, "</td>");
fprintf( fil, "<td %s>", STYLE_TD_HOURS ); write_hour( t, fil ); fprintf( fil, "-");
t += ( durslots * SECperSLOT ); write_hour( t, fil ); fprintf( fil, "</td>");
// la salle
fprintf( fil, "<td %s>", STYLE_TD_ROOM );
set<int>::iterator begin=resourceIDs.begin(), end=resourceIDs.end();
while	( begin != end )
	{
	rid = *(begin++);
	re = ja->tr.get_resource_by_ID(rid);
	if	( re )
		{
		if	( re->category == Room )
			{
			rname = re->name.c_str();
			fprintf( fil, "%.7s ", rname );	// nom de salle tronque a 7 char s !!!!!!!
			}
		}
	}
fprintf( fil, "</td>\n");
// le titre
fprintf( fil, "<td %s>%s", STYLE_TD_TITLE, getname(ja)->c_str() );
if	( i )
	fprintf( fil, " %2d/%d", i, j );
fprintf( fil, "</td>\n" );
// le prof (ou les profs)
if	( cat != Instructor )
	{
	fprintf( fil, "<td %s>", STYLE_TD_TRAINER );
	begin = resourceIDs.begin();
	while	( begin != end )
		{
		rid = *(begin++);
		re = ja->tr.get_resource_by_ID(rid);
		if	( re )
			{
			if	( re->category == Instructor )
				{
				rname = re->name.c_str();
				fprintf( fil, "%s ", rname );
				}
			}
		}
	fprintf( fil, "</td>\n");
	}
// le groupe (ou les groupes)
if	( cat != Trainee )
	{
	fprintf( fil, "<td %s>", STYLE_TD_TRAINEE );
	begin = resourceIDs.begin();
	while	( begin != end )
		{
		rid = *(begin++);
		re = ja->tr.get_resource_by_ID(rid);
		if	( re )
			{
			if	( re->category == Trainee )
				{
				rname = re->name.c_str();
				fprintf( fil, "%s ", rname );
				}
			}
		}
	fprintf( fil, "</td>\n");
	}
fprintf( fil, "</tr>\n");
}

void table_events::gen_html_head_for_1_cat( FILE * fil, jade_appli *ja, string name, CATEGORY cat )
{
switch	( cat )
	{
	case Trainee	: fprintf( fil, "<h1>Group "); break;
	case Instructor	: fprintf( fil, "<h1>Instructor "); break;
	case Room	: fprintf( fil, "<h1>Room "); break;
	default		: fprintf( fil, "<h1>"); break;
	}
fprintf( fil, "%s</h1>\n", name.c_str() );
fprintf( fil, "<table %s><tr><td %s>Date</td><td %s>Hours</td><td %s>Room</td><td %s>Title</td>",
		STYLE_TABLE, STYLE_TD_HEAD_DATE, STYLE_TD_HEAD_HOURS, STYLE_TD_HEAD_ROOM, STYLE_TD_HEAD_TITLE );
if	( cat != Instructor )
	fprintf( fil, "<td %s>Instructor</td>", STYLE_TD_HEAD_TRAINER );
if	( cat != Trainee )
	fprintf( fil, "<td %s>Group</td>", STYLE_TD_HEAD_TRAINEE );
fprintf( fil, "</tr>\n");
}

void table_events::gen_html_for_1_res( FILE * fil, jade_appli *ja, int ID )
{
resource * re; CATEGORY cat;
re = ja->tr.get_resource_by_ID(ID);
if	( re == NULL )
	return;
cat = re->category;
gen_html_head_for_1_cat( fil, ja, re->name, cat );
unsigned int i; event * eve;
for	( i = 0; i < trindex.size(); ++i )
	{
	eve = get_event_by_ID( trindex.at(i) );
	if	( eve->resourceIDs.count( ID ) )
		{
		eve->gen_html_row( fil, ja, ID, cat, ( i == 0 ), 0, 0 );
		}
	}
fprintf( fil, "</table>\n" );
}

void table_events::gen_html_file_for_1_res( const char * path, jade_appli *ja, int ID )
{
resource * re; char fnam[256];
re = ja->tr.get_resource_by_ID(ID);
if	( re == NULL )
	return;
if	( path )
	snprintf( fnam, sizeof(fnam), "%s\%s.html", path, re->name.c_str() );
else	snprintf( fnam, sizeof(fnam), "%s.html", re->name.c_str() );
FILE * fil = fopen( fnam, "w" );
if	( fil )
	{
	fprintf( fil, "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n" );
	fprintf( fil, "<html><head><title>Groupe %s</title>\n", re->name.c_str() );
	fprintf( fil, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=ISO-8859-1\">\n" );
	fprintf( fil, "</head><body %s>\n", STYLE_BODY );
	gen_html_for_1_res( fil, ja, ID );
	fclose( fil );
	}
}

void selection::gen_html( FILE * fil, jade_appli *ja )
{
string titre = string("Selection pour ")
	+ *(ja->tr.get_resource_name( ressourceID ))
	+ string(", \"") + regexp + string("\"");
ja->te.gen_html_head_for_1_cat( fil, ja, titre, (CATEGORY)(-1) );
// fprintf( fil, "nombre d'events %d\n", events.size() );
unsigned int i; event * eve;
for	( i = 0; i < events.size(); ++i )
	{
	eve = ja->te.get_event_by_ID( events[i] );
	if	( eve )
		eve->gen_html_row( fil, ja, ressourceID, (CATEGORY)(-1), ( i == 0 ), i+1, events.size() );
	}
fprintf( fil, "</table>\n" );
}

// dump HTML selon liste
void jade_appli::gen_html_multi_select( FILE * fil, vector<selection> * liste ) {
   vector<selection>::iterator begin, end;
   begin = liste->begin();
   end = liste->end();
   while ( begin != end )
	 {
	 begin->clear();
	 begin->select( this );
	 fprintf( fil, "<hr />\n" );
	 begin->gen_html( fil, this );
	 ++begin;
	 }
   }

//* ============================== THE DUMP AREA ==================================== **/

void activity::dump1( FILE * fil ) {
   fprintf( fil, "%5d %2dsl %2dpl %s%s\n", ID, durslots, placedslots, path.c_str(), name.c_str() );
   }
void activity::dump2( FILE * fil ) {
   fprintf( fil, "%5d %2dsl %s%s\n      %2dpl ", ID, durslots, path.c_str(), name.c_str(), placedslots );
   set<int>::iterator begin=resourceIDs.begin(), end=resourceIDs.end();
   while ( begin != end )
	 fprintf( fil, "%d ", *(begin++) );
   fprintf( fil, "( " );
   begin=resourceOrIDs.begin(), end=resourceOrIDs.end();
   while ( begin != end )
	 fprintf( fil, "%d ", *(begin++) );
   fprintf( fil, ")\n" );
   }
void activity::dump3( FILE * fil, jade_appli *ja ) {
   int rid; resource * re; const char * rname;
   fprintf( fil, "%5d %2dsl %s%s\n", ID, durslots, path.c_str(), name.c_str() );
   set<int>::iterator begin=resourceIDs.begin(), end=resourceIDs.end();
   while ( begin != end )
	 {
	 rid = *(begin++);
	 re = ja->tr.get_resource_by_ID(rid);
	 if	( re )
		rname = re->name.c_str();
	 else	rname = "error_no_resource_found";
	 fprintf( fil, "           %5d %s\n", rid, rname );
	 }
   begin=resourceOrIDs.begin(), end=resourceOrIDs.end();
   while ( begin != end )
	 {
	 rid = *(begin++);
	 re = ja->tr.get_resource_by_ID(rid);
	 if	( re )
		rname = re->name.c_str();
	 else	rname = "error_no_resource_found";
	 fprintf( fil, "           (%5d %s)\n", rid, rname );
	 }
   }

void event::dump( FILE * fil, jade_appli *ja, int flag, int i, int j ) {
   int rid; CATEGORY cat;
   // 1ere ligne
   if	( flag & DUMP_WEEK )					// semaine
	fprintf( fil, "s%02d ", ja->ade2week( absslot ) );
   if	( flag & DUMP_TIME )					// date et heures
	{
	unsigned int t = ja->ade2time( absslot );
	write_jdate( t, fil, '/' ); fprintf( fil, " ");
	write_hour( t, fil ); fprintf( fil, "-");
	t += ( durslots * SECperSLOT );
	write_hour( t, fil );
	}							// info brute
   else fprintf( fil, "%5d %5dsl %2dsl ", ID, absslot, durslots );
   if	( flag & DUMP_NAME )					// le nom
	{
	fprintf( fil, " ");
	write_clean_name( getname(ja)->c_str(), fil );
	}
   if	( flag & DUMP_FRACTION )				// fraction
	fprintf( fil, " %2d/%d", i, j );
   fprintf( fil, "\n" );
   // ressources
   set<int>::iterator begin=resourceIDs.begin(), end=resourceIDs.end();
   if	( flag & DUMP_RAW_RES )					// ressources brutes
	{
	while	( begin != end )
		fprintf( fil, "%d ", *(begin++) );
	fprintf( fil, "\n" );
	}
   if	( flag & ( DUMP_TRAINEE	| DUMP_INSTRUC | DUMP_ROOM | DUMP_EQUIP ) ) // ressources par categorie
	{
	while	( begin != end )
		{
		rid = *(begin++);
		resource * re = ja->tr.get_resource_by_ID( rid );
		if	( re )
			{
			cat = re->category;
			if	(
				   ( ( flag & DUMP_TRAINEE ) && ( cat == Trainee ) )
				|| ( ( flag & DUMP_INSTRUC ) && ( cat == Instructor ) )
				|| ( ( flag & DUMP_ROOM    ) && ( cat == Room ) )
				|| ( ( flag & DUMP_EQUIP   ) && ( cat == Equipment ) )
				)
				{
				if	( flag & DUMP_ID )
					fprintf( fil, "   %5d ", rid );
				else	fprintf( fil, "   ");
				write_clean_name( re->name.c_str(), fil );
				fprintf( fil, "\n");
				}
			}
		}
	}
   }

/*
emission texte compatible module LCD STM32F7, style 2018, exemple
"13.07.36TD Complement de Mecanique Quantique.034AE.12PLANTEC J.Y..ffD080;";
*/
#define DOOR_C_STYLE	// pour mettre le texte dans le src C du proto STM32
void event::door_dump( FILE * fil, jade_appli *ja )
{
int cnt, rid;
#ifdef DOOR_C_STYLE
fprintf( fil, "\"" );
#endif
// time
fprintf( fil, "%02d.%02d.", absslot % SLOTperDAY, durslots );
// title - d'abord compter les chars
cnt = write_clean_name( getname(ja)->c_str(), NULL );
fprintf( fil, "%02d", cnt );
write_clean_name( getname(ja)->c_str(), fil );
fprintf( fil, "." );
// ressources : trainees (N.B. il peut y en avoir plusieurs !)
set<int>::iterator begin, end = resourceIDs.end();
resource * re; string txt;
begin = resourceIDs.begin();
cnt = 0;
while	( begin != end )
	{
	rid = *(begin++);
	re = ja->tr.get_resource_by_ID( rid );
	if	( re )
		{
		if	( re->category == Trainee )
			{
			if	( cnt++ )
				txt += string(",");
			txt += re->name;
			}
		}
	}
cnt = write_clean_name( txt.c_str(), NULL );
fprintf( fil, "%02d", cnt );
write_clean_name( txt.c_str(), fil );
fprintf( fil, "." );
// ressources : profs (N.B. il peut y en avoir plusieurs !)
begin = resourceIDs.begin();
cnt = 0; txt = string("");
while	( begin != end )
	{
	rid = *(begin++);
	re = ja->tr.get_resource_by_ID( rid );
	if	( re )
		{
		if	( re->category == Instructor )
			{
			if	( cnt++ )
				txt += string(",");
			txt += re->name;
			}
		}
	}
cnt = write_clean_name( txt.c_str(), NULL );
fprintf( fil, "%02d", cnt );
write_clean_name( txt.c_str(), fil );
fprintf( fil, "." );
// couleur
fprintf( fil, "%06x", color );
fprintf( fil, ";");
#ifdef DOOR_C_STYLE
fprintf( fil, "\"\n" );
#endif
}

/*
emission texte compatible module LCD STM32F7, style 2018, exemple
"@213.07.36TD Complement de Mecanique Quantique.034AE.12PLANTEC J.Y..ffD080;";
*/
void selection::door_dump( FILE * fil, jade_appli *ja )
{
fprintf( fil, "Selection pour %d (%s), \"%s\"\n",
	 ressourceID, ja->tr.get_resource_name( ressourceID )->c_str(), regexp.c_str() );
unsigned int i, cursem, curday, islot, isem, iday; event * eve;
cursem = curday = -1;
for	( i = 0; i < events.size(); ++i )
	{
	eve = ja->te.get_event_by_ID( events[i] );
	if	( eve )
		{
		// separation jour ou semaine
		islot = eve->absslot % (SLOTperDAY*7);	// slot relatif au debut de la semaine
		iday = islot / SLOTperDAY;		// jour de la semaine
		isem = ja->ade2week( eve->absslot );
		if	( cursem != isem )
			{
			fprintf( fil, "\nWeek %d\n", isem  );
			cursem = isem;
			curday = -1;	// forcer changt jour
			}
		if	( curday != iday )
			{
			#ifdef DOOR_C_STYLE
			fprintf( fil, "\"@%d\"\n", iday );
			#else
			fprintf( fil, "@%d", iday );
			#endif
			curday = iday;
			}
		// dump du jour
		eve->door_dump( fil, ja );
		}
	}
}

int selection::dump( FILE * fil, jade_appli *ja, int flag ) {
  fprintf( fil, "Selection pour %d (%s), \"%s\"\n",
	   ressourceID, ja->tr.get_resource_name( ressourceID )->c_str(), regexp.c_str() );
  // fprintf( fil, "nombre d'events %d\n", events.size() );
  unsigned int i; int durslots, slotsCM, slotsTP, seCM, seTP, errcnt; event * eve;
  slotsCM = slotsTP = seCM = seTP = errcnt = 0;

  for	( i = 0; i < events.size(); ++i )
	{
	eve = ja->te.get_event_by_ID( events[i] );
	if	( eve )
		{
		if	( flag & DUMP_EVENT )
			eve->dump( fil, ja, flag, i+1, events.size() );
		durslots = eve->durslots;
		if	( durslots == 5 )
			{
			slotsCM += durslots; ++seCM;
			}
		else	{
			slotsTP += durslots; ++seTP;
			}
		}
	}
  if	( flag & DUMP_CM_TP )
	fprintf( fil, "CM : %ds = %gh, TP : %ds = %gh\n", seCM, (double)slotsCM * HOURperSLOT,
							  seTP, (double)slotsTP * HOURperSLOT );
  if	( flag & DUMP_TOTAL )
	{
	fprintf( fil, "total %gh / %gh ", (double)(slotsCM+slotsTP) * HOURperSLOT, totalslots * HOURperSLOT );
	if	( (slotsCM+slotsTP) == totalslots )
		{ fprintf( fil, "Ok\n" ); errcnt = 0; }
	else if	( (slotsCM+slotsTP) < totalslots )
		{ fprintf( fil, "MANQUE %gh ---------------------------\n",
				(double)(totalslots-(slotsCM+slotsTP)) * HOURperSLOT ); errcnt = 1; }
	else	{ fprintf( fil, "EXCES %gh ++++++++++++++++++++++++++++\n",
				(double)((slotsCM+slotsTP)-totalslots) * HOURperSLOT ); errcnt = 1; }
	}
  return errcnt;
  }

void jade_appli::dump_used_res( FILE * fil ) {
   int rid; resource * re; const char * rname;
   map<int,int>::iterator begin=used_resources.begin(), end=used_resources.end();
   fprintf( fil, "trainees :\n");
   while ( begin != end )
	 {
	 rid = (begin++)->first;
	 re = tr.get_resource_by_ID(rid);
	 if	( re )
		if	( re->category == Trainee )
			{
			rname = re->name.c_str();
			fprintf( fil, "  %5d %s\n", rid, rname );
			}
	 }
   fprintf( fil, "Instructors :\n");
   begin=used_resources.begin();
   while ( begin != end )
	 {
	 rid = (begin++)->first;
	 re = tr.get_resource_by_ID(rid);
	 if	( re )
		if	( re->category == Instructor )
			{
			rname = re->name.c_str();
			fprintf( fil, "  %5d %s\n", rid, rname );
			}
	 }
   fprintf( fil, "Rooms :\n");
   begin=used_resources.begin();
   while ( begin != end )
	 {
	 rid = (begin++)->first;
	 re = tr.get_resource_by_ID(rid);
	 if	( re )
		if	( re->category == Room )
			{
			rname = re->name.c_str();
			fprintf( fil, "  %5d %s\n", rid, rname );
			}
	 }
   fprintf( fil, "Unknown :\n");
   begin=used_resources.begin();
   while ( begin != end )
	 {
	 rid = (begin++)->first;
	 re = tr.get_resource_by_ID(rid);
	 if	( re == NULL )
		fprintf( fil, "%5d\n", rid );
	 }
   }

// dump de selections selon liste
int jade_appli::dump_multi_select( FILE * fil, vector<selection> * liste, int flag ) {
   vector<selection>::iterator begin, end;
   begin = liste->begin();
   end = liste->end();
   int errcnt = 0;
   while ( begin != end )
	 {
	 begin->clear();
	 begin->select( this );
	 fprintf( fil, "\n" );
	 errcnt += begin->dump( fil, this, flag );
	 ++begin;
	 }
   return errcnt;
   }
