/*
g++ -Wall ssplit2.cpp -o ssplit2
*/
#include <string>
#include <vector>
using namespace std;

#include "ssplit2.h"

// une fonction qui coupe la string 'strin' en morceaux selon le
// delimiteur 'deli', et range les morceaux non vides dans le vector 'splut'
// qui doit exister. rend le nombre de morceaux.
int ssplit2( vector <string> * splut, string strin, char deli )
{
unsigned int pos, cnt; string part;

pos = 0; cnt = 0; splut->clear();
while ( pos < strin.size() )
   {
   if   ( strin[pos] == deli )
	{
	if ( part.size() )
	   {
	   splut->push_back( part );
	   part = string("");
	   cnt++;
	   }
	}
   else part += strin[pos];
   pos++;
   } 
if ( part.size() )
   {
   splut->push_back( part );
   part = string("");
   cnt++;
   }
return cnt;
}

// une fonction qui coupe la string 'strin' en morceaux selon les blancs
// et range les morceaux non vides dans le vector 'splut'
// qui doit exister. rend le nombre de morceaux.
// blancs, tabs et non imprimables sont elimines
// un morceau contenant des blancs ou quotes peut etre escape par une paire de l'autre quote
int ssplit2( vector <string> * splut, string strin )
{
unsigned int pos, cnt; string part; char c, quote;

pos = 0; cnt = 0; splut->clear(); quote = 0;
while	( pos < strin.size() )
	{
	c = strin[pos];
	if	( ( quote == 0 ) && ( ( c == '"' ) || ( c == '\'' ) ) )
		{ quote = c; c = 0; }	// opening quote
	else if	( c == quote )
		{ quote = 0; c = 0; }	// closing quote
	if	( ( c == 0 ) || ( ( quote == 0 ) && ( c <= ' ' ) ) )
		{
		if ( part.size() )
			{
			splut->push_back( part );
			part = string("");
			cnt++;
			}
		}
	else	part += c;
	pos++;
	}
if	( part.size() )
	{
	splut->push_back( part );
	cnt++;
	}
return cnt;
}

/* test *
#include <iostream>
int main( int argc, char ** argv )
{
vector <string> resu; int iarg, i, n;
if	( argc >= 2 )
	{
	for	( iarg = 1; iarg < argc; ++iarg )
		{
		// n = ssplit2( &resu, argv[iarg], '+' );
		n = ssplit2( &resu, argv[iarg] );
		cout << n << " morceaux\n";
		for	( i = 0; i < n; i++ )
			cout << "\t" << resu[i] << endl;
		}
	}
else	{
	string demo = string("pipo\t'pi po'  \"single single '\" 'single double \"' ");
	n = ssplit2( &resu, demo );
	cout << demo << "\n" << n << " morceaux\n";
	for	( i = 0; i < n; i++ )
		cout << "\t" << resu[i] << endl;
	}
return 0;
}
//*/
