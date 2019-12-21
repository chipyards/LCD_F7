// declaration de donnees extern crees dans jfonts.c
// les types sont definis dans jlcd.h
extern const unsigned short JFont24_Table[];
extern const unsigned short JFont20_Table[];
extern const unsigned short JFont16_Table[];
extern const unsigned short JFont12_Table[];
extern const unsigned short JFont8_Table[];
extern const JFONT JFont24;
extern const JFONT JFont20;
extern const JFONT JFont16;
extern const JFONT JFont16n;
extern const JFONT JFont12;
extern const JFONT JFont8;

#ifdef COMPILE_THE_FONTS
// adresses des tables de rows compilees, placees au bon vouloir du compilateur
#define JFONT_FONTS_24 JFont24_Table
#define JFONT_FONTS_20 JFont20_Table
#define JFONT_FONTS_16 JFont16_Table
#define JFONT_FONTS_12 JFont12_Table
#define JFONT_FONTS_8  JFont8_Table
#else
// adresses des tables de rows reflashees, placees dans l'ordre grace a flash_the_fonts()
#define JFONT_FONTS_24 FLASH_FONTS_BASE
#define JFONT_FONTS_20 (FLASH_FONTS_BASE+(2*21*QCHAR))
#define JFONT_FONTS_16 (FLASH_FONTS_BASE+(2*21*QCHAR)+(2*17*QCHAR))
#define JFONT_FONTS_12 (FLASH_FONTS_BASE+(2*21*QCHAR)+(2*17*QCHAR)+(2*13*QCHAR))
#define JFONT_FONTS_8  (FLASH_FONTS_BASE+(2*21*QCHAR)+(2*17*QCHAR)+(2*13*QCHAR)+(2*10*QCHAR))
#endif
