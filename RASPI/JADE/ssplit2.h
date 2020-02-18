
// une fonction qui coupe la string 'strin' en morceaux selon le
// delimiteur 'deli', et range les morceaux non vides dans le vector 'splut'
// qui doit exister. rend le nombre de morceaux.
int ssplit2( vector <string> * splut, string strin, char deli );

// une fonction qui coupe la string 'strin' en morceaux selon les blancs
// et range les morceaux non vides dans le vector 'splut'
// qui doit exister. rend le nombre de morceaux.
// blancs, tabs et non imprimables sont elimines
// un morceau contenant des blancs ou quotes peut etre escape par une paire de l'autre quote
int ssplit2( vector <string> * splut, string strin );
