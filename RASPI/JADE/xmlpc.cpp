/* parseur XML version C
   - derive du parseur en C de VirVol 1 a 14
   - ignore le texte dans le contenu des elements
   - maintient une pile de l'ascendance de l'element courant
 */
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include "xmlpc.h"

using namespace std;

// xelem method bodies
void xelem::xmlout( ostream & os )
{	// emettre du xml...
os << "<" << tag;
map<string,string>::iterator p;
for ( p = attr.begin(); p != attr.end(); p++ )
    {
    os << " " << p->first << "=\"" << p->second << "\"";
    }
/*
if   ( text.size() ) os << ">\n" << text << "\n</" << tag << ">\n";
else                 os << " />\n";
*/
os << ">\n";
}

// xmlobj method bodies
/* le parseur : lit le texte jusqu'a pouvoir retourner :
   0 si EOF
   1 creation d'un nouvel element (tous attributs inclus, contenu exclu)
     accessible au sommet de la pile (this->stac)
   2 fin de l'element courant (encore accessible au sommet de la pile,
     mais en instance d'etre depile)
   <0 si erreur

etats :
-1 : hors tag, depilage immediat requis
 0 : hors tag
 1 : vu <, attente nom
 2 : nom en cours
 3 : espace apres nom, attente nom ou / ou >
 4 : nom attrib en cours
 5 : vu =, attente quote
 6 : vu ", val en cours
 7 : vu / en fin d'empty tag, attente >
 10: vu </, attente nom end tag en cours
 11: vu nom end tag, attente >
 20: vu <?, attente fin PI soit ?
 21: vu ?, attente >
 30: vu <!, attente -- ou [ ou "
     (on essaie d'etre transparent a ENTITY ou [CDATA[ ou DOCTYPE ou ATTLIST
     ou ELEMENT ou NOTATION)
 31: vu <!, attente [ ou " ou >
 40: vu <!-, attente -
 41: vu <!-- attente -
 42: vu <!-- -, attente -
 43: vu <!-- --, attente >
 50: vu [ dans <!, attente ], ignorer tout sauf "
 51: vu " dans <![, attente ", ignorer tout le reste
 52: vu " dans <!, attente ", ignorer tout le reste

BUGs : (pas graves)
	- ne devrait pas tolerer blanc entre < et nom tag
	- devrait tolerer blanc entre nom attr et =
 */
int xmlobj::step()
{

int c;		// caractere courant, int pour percevoir EOF
string nam;	// nom tag ou attribut courant dans ce step
string val;	// valeur attribut courant dans ce step

if ( e == -1 )	// depilage eventuel avant lecture char
   {
   if   ( stac.size() )
	stac.pop_back();
   else return(-667);
   e = 0;
   }

while ( ( c = is.get() ) != EOF )
   {
   if ( c == 10 ) curlin++;
   switch (e)
      {
      case 0 : if ( c == '<' )
	          {
		  e = 1; nam = "";
		  }			break;
      case 1 : if ( c == '/' )
                  {
		  e = 10;
		  nam = "";
		  }		       else
               if ( c == '?' ) e = 20; else
	       if ( c == '!' ) e = 30; else
               if ( c > ' ' )
	          {
		  e = 2;
		  nam = char(c);
		  }			break;
      case 2 : if   ( ( c == '>' ) || ( c == '/' ) || ( c <= ' ' ) )
		    {
		    // printf(" element %s\n", nam.c_str() );
		    if	( pDTD )
			{
			if ( pDTD->elem.count(nam) == 0 )
			return( -1983 );
			}
		    stac.push_back( xelem(nam) );
		    if  ( c == '>' )
			{ e = 0; return(1); }
		    if  ( c == '/' )
			{ e = 7; return(1); }
		    e = 3;
		    }
	       else nam += char(c);
					break;

	    /* if ( c == '>' )
                  {
	          e = 0;
		  stac.push_back( xelem(nam) );
		  return(1);
		  }                     else
               if ( c == '/' )
                  {
	          e = 7;
		  stac.push_back( xelem(nam) );
		  return(1);
		  }                     else
               if ( c > ' ' )
		  {
		  nam += char(c);
		  }			else
	          {
	          e = 3;
		  stac.push_back( xelem(nam) );
		  }                    break;
	     */
      case 3 : if ( c == '>' )
                  {
	          e = 0;
		  return(1);
		  }                     else
               if ( c == '/' )
                  {
	          e = 7;
		  return(1);
		  }                     else
               if ( c > ' ' )
		  {
		  e = 4;
		  nam = char(c);
		  }                    break;
      case 4 : if ( c == '=' )
                  {
	          e = 5;
		  // printf("   element %s attribut %s\n", stac.back().tag.c_str(), nam.c_str() );
		  if ( pDTD )
		     {
		     if ( pDTD->elem[stac.back().tag].attrib.count(nam) == 0 )
			return( -1984 );
		     }
		  stac.back().attr[nam] = "";
		  }                     else
               if ( c > ' ' )
		  {
		  nam += char(c);
		  }			else
	          return(-400);         break;		  
      case 5 : if ( c == '"' )
                  {
	          e = 6;
		  val = "";
		  }                     else
               if ( c > ' ' ) return(-500);
	                               break;
      case 6 : if ( c == '"' )
                  {
	          e = 3;
		  stac.back().attr[nam] = val;
		  }                     else
		  {
		  val += char(c);
		  }            break;		  
      case 7 : if ( c == '>' )
                  {
	          e = -1;
                  return(2);
		  }                     else
                  return(-700);         break;		  
      case 10: if ( c == '>' )
                  {
	          e = -1;
		  if ( nam != stac.back().tag )
		     return(-1001);
                  return(2);
		  }                     else
               if ( c > ' ' )
                  {
		  nam += char(c);
		  }			else
		  e = 11;		break;		  
      case 11: if ( c == '>' )
                  {
	          e = -1;
		  if ( nam != stac.back().tag )
		     return(-1001);
                  return(2);
		  }                     else
	       if ( c > ' ' )
		  return(-1100);	break;	  	
      case 20: if ( c == '?' )
	          e = 21;		break;
      case 21: if ( c == '>' )
	          e = 0;                else
                  e = 20;		break;
      case 30: if ( c == '-' )
       	          e = 40;               else
	       if ( c == '[' )
       	          e = 50;               else
	       if ( c == '"' )
       	          e = 52;               else
                  e = 31;              break;
      case 31: 
               if ( c == '>' )
	          e = 0;                else
	       if ( c == '[' )
       	          e = 50;               else
	       if ( c == '"' )
       	          e = 52;               else
                  e = 31;              break;
      case 40: if ( c == '-' )
       	          e = 41;               else
                  return(-4000);        break;
      case 41: if ( c == '-' )
       	          e = 42;              break;
      case 42: if ( c == '-' )
       	          e = 43;               else
		  e = 41;              break;
      case 43: if ( c == '>' )
	          e = 0;                else
		  e = 41;              break;
      case 50: 
               if ( c == ']' )
       	          e = 31;               else
	       if ( c == '"' )
       	          e = 51;              break;
      case 51: if ( c == '"' )
       	          e = 50;              break;
      case 52: if ( c == '"' )
       	          e = 31;              break;
      default: return(-666);
      }
   /* printf("c='%c'  e=%d\n", c, e ); */
   }
// si on est ici on a atteint la fin du fichier
return(0);
}
