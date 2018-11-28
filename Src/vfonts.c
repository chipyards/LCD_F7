/* fonts dependant d'une BMP vfonts1.bin chargee en flash @ 0x08040000
   les caracteres sont a largeur variable mais on a maintenu une largeur uniforme pour les chiffres
 */

// Chaparral Pro Bold 48pt rendu a 75 DPI par PS
// interligne PS : 59px (48pt = 50px theoriques)
// 14 cells "0123456789:;<=" avec ';' --> "Salle", '<' --> logo INSA, '=' --> burger icon
// les chiffres font 33px de haut mais on a mis 36 a cause des 'l' de "Salle"
const JVCHAR JVFont36n_defs[14] = 
{
{ 1, 4, 19, 36 },
{ 22, 4, 19, 36 },
{ 44, 4, 19, 36 },
{ 65, 4, 19, 36 },
{ 87, 4, 19, 36 },
{ 108, 4, 19, 36 },
{ 130, 4, 19, 36 },
{ 151, 4, 19, 36 },
{ 173, 4, 19, 36 },
{ 194, 4, 19, 36 },
{ 216, 4, 9, 36 },
{ 230, 4, 82, 36 },
{ 7, 166, 154, 61 },
{ 261, 55, 48, 40 }
};

const JVFONT JVFont36n = {
0x08040000,		// adresse data (ici bmp entiere)
JVFont36n_defs,		// tableau des dimensions des caracteres
'0',			// code ascii du premier char
14,			// nombre de chars
3,			// char separation (hint)
42			// line spacing
}; 

// Chaparral Pro Bold 36pt rendu a 75 DPI par PS
// 16 cells "0123456789:;<=>?" avec ':' --> '-' (pour date)
// ';' --> "Lundi" etc... '?' = "Vendredi"
const JVCHAR JVFont26s_defs[16] = 
{
{ 5, 69, 14, 26 },
{ 21, 69, 14, 26 },
{ 37, 69, 14, 26 },
{ 53, 69, 14, 26 },
{ 69, 69, 14, 26 },
{ 86, 69, 14, 26 },
{ 102, 69, 14, 26 },
{ 118, 69, 14, 26 },
{ 134, 69, 14, 26 },
{ 150, 69, 14, 26 },
{ 166, 69, 9, 26 },
{ 178, 69, 73, 26 },
{ 6, 134, 77, 26 },
{ 96, 102, 82, 26 },
{ 91, 134, 70, 26 },
{ 6, 102, 84, 26 }
};

const JVFONT JVFont26s = {
0x08040000,		// adresse data (ici bmp entiere)
JVFont26s_defs,		// tableau des dimensions des caracteres
'0',			// code ascii du premier char
16,			// nombre de chars
2,			// char separation (hint)
30			// line spacing
}; 

// Chaparral Pro Bold 28pt rendu a 75 DPI par PS
// 11 cells "0123456789:"
const JVCHAR JVFont19n_defs[11] = 
{
{ 4, 45, 12, 19 },
{ 17, 45, 12, 19 },
{ 29, 45, 12, 19 },
{ 42, 45, 12, 19 },
{ 54, 45, 12, 19 },
{ 67, 45, 12, 19 },
{ 80, 45, 12, 19 },
{ 93, 45, 12, 19 },
{ 105, 45, 12, 19 },
{ 118, 45, 12, 19 },
{ 130, 45, 6, 19 }
};

const JVFONT JVFont19n = {
0x08040000,		// adresse data (ici bmp entiere)
JVFont19n_defs,		// tableau des dimensions des caracteres
'0',			// code ascii du premier char
11,			// nombre de chars
2,			// char separation (hint)
24			// line spacing
}; 



