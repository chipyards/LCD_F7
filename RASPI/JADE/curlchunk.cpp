// derive de curl_allexamples/getinmemory.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pcre.h>
#include <curl/curl.h>

using namespace std;
#include <string>

#include "curlchunk.h"
#include "pcreux.h"

// la call back pour etre appelee par la lib curl
extern "C" {
static size_t WriteMemoryCallback( void *contents, size_t size, size_t nmemb, void *userp )
{
curlchunk * chunk = (curlchunk *)userp;
size_t addsize = size * nmemb;
// printf("%d x %d\n", size, nmemb );
size_t newsize = chunk->size + addsize;
// allouer plus de memoire si necessaire
chunk->check_alloc( newsize + 1 );
// on a assez de memoire, on copie
memcpy(&( chunk->data[chunk->size]), contents, addsize );
chunk->size += addsize;
chunk->data[chunk->size] = 0;
return addsize;
}
}	// extern "C"

// reallocation exponentielle (alloc initiale est dans le constructeur)
void curlchunk::check_alloc( size_t newsize )
{
while	( newsize > space )
	{
	data = (char *)realloc( (void *)data, space * 2 );
	if	( data == NULL )
		{			/* out of memory! */
		printf("not enough memory (realloc returned NULL)\n");
		exit(EXIT_FAILURE);
		}
	space *= 2;
	printf("reallocated %d\n", (int)space );
	}
}

// la methode pour recuperer les data sur le web
int curlchunk::download( const char * fullurl )
{
  CURL *curl_handle;

  this->size = 0;	// effacer contenu precedent
  this->data[0] = 0;

  curl_global_init(CURL_GLOBAL_ALL);

  /* init the curl session */
  curl_handle = curl_easy_init();

  /* specify URL to get */
  curl_easy_setopt(curl_handle, CURLOPT_URL, fullurl );

  /* JLN's debug tools *
  printf("URL = %s\n", fullurl );
  curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1 );
  //*/

  /* send all data to this function  */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

  /* we pass our 'chunk' struct to the callback function */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)this );

  /* some servers don't like requests that are made without a user-agent
     field, so we provide one */
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/7.24");

  /* option -k aka insecure, ajoutee pour https
     N.B. contrairement au code fourni par curl --libcurl, CURLOPT_SSL_VERIFYHOST ne suffit pas */
  curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0 );
  curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0 );

  /* get it! */
  curl_easy_perform(curl_handle);

  /* cleanup curl stuff */
  curl_easy_cleanup(curl_handle);

  printf("curl : %d bytes retrieved\n", (int)this->size );

  /* we're done with libcurl, so clean it up */
  curl_global_cleanup();

  return this->size;
}

// la methode pour recuperer les data sur le web, avec host et path predefinis
int curlchunk::download( string query_string )
{
string full = ade_host + ade_path + query_string;
return( download( full.c_str() ) );
}

// ouverture de session, utilise projectid, renseigne sessionid
void curlchunk::opensession()
{
// premier pas : connexion
download( string("function=connect&") + login_pwd );
if	( size )
	{
	// fwrite( data, 1, (int)size, stdout );
	pcreux * lareu = new pcreux( "session id=\"([^\"]+)\"" );
	if	( lareu->matchav( data, size ) == 2 )
		{
		int start = lareu->ovector[2];
		int stop  = lareu->ovector[3];
		data[stop] = 0;
		// printf("session id = [%s]\n", &data[start] );
		sessionid = string( &data[start] );
		}
	delete( lareu );
	}
/* premier pas et demi : listing des projets */
download( string("sessionId=") + sessionid + string("&function=getProjects&detail=4") );
if	( size )
	{
	fwrite( data, 1, (int)size, stdout );
	}

// second pas : choix du projet
download( string("sessionId=") + sessionid + string("&function=setProject&projectId=" + projectid ) );
if	( size )
	{
	fwrite( data, 1, (int)size, stdout );
	}
else	sessionid = string("");
}

void curlchunk::closesession()
{
if	( sessionid.size() )
	{
	download( string("sessionId=") + sessionid + string("&function=disconnect") );
	if	( size )
		fwrite( data, 1, (int)size, stdout );
	if	( data )
		{ free( data ); size = 0; space = 0; }
	}
sessionid = string("");
}

/* generation d'un nom de fichier xml (xmlpath vient avec le / ou \ terminal)
void funcode2fnam( string funcode, string xmlpath, char * fnam, int size )
{
snprintf( fnam, size, "%s%s.xml", xmlpath.c_str(), funcode.c_str() );
unsigned int i;
for	( i = 0; i < strlen( fnam ); ++i )
	{
	if ( fnam[i] == '&' ) fnam[i] = '_';
	if ( fnam[i] == '=' ) fnam[i] = '-';
	}
}
*/

// cette fonction s'assure de l'existence d'un fichier xml associe a un funcode,
// le telechargeant si necessaire ou si force
// 	download_flag = -1 : fichier xml read-only (echec si fichier absent)
//	download_flag =  0 : automatique : download si fichier absent, precede de opensession si necessaire,
//	download_flag =  1 : force download
// retour : pathname complet ou "" si echec
string curlchunk::grab_xml( string & xmlpath, string & funcode, int download_flag )
{
// char fnam[128];
// funcode2fnam( funcode, fnam, sizeof( fnam ) );
string fnam;
fnam = xmlpath + funcode + string(".xml");
unsigned int i;
for	( i = 0; i < fnam.size(); ++i )
	{
	if ( fnam[i] == '&' ) fnam[i] = '_';
	if ( fnam[i] == '=' ) fnam[i] = '-';
	}
if	( download_flag != 1 )
	{
	FILE * lefil = fopen( fnam.c_str(), "r" );
	if	( lefil )
		{
		fclose( lefil );
		printf("file %s already here\n", fnam.c_str() );
		return fnam;
		}
	if	( download_flag < 0 )
		return string("");
	}
if	( sessionid.size() == 0 )
	{
	opensession();
	if	( sessionid.size() )
		printf("session %s started\n", sessionid.c_str() );
	else	return string("");
	}
download( string("sessionId=") + sessionid + string("&function=") + funcode );
if	( size )
	{
	FILE * lefil;
	lefil = fopen( fnam.c_str(), "w" );
	if	( lefil == NULL )
		return string("");
	fwrite( data, 1, (int)size, lefil );
	fclose( lefil );
	}
printf("file %s downloaded, %d bytes\n", fnam.c_str(), (int)size );
return fnam;
}
