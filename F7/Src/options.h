#ifdef __GNUC__
#define VERSION "3.6 gnu"
#else
#define VERSION "3.6 keil"
#endif

// #define GREEN_CPU		// sleep dans main loop

/** modules optionnels du display **/
#define USE_UART1		// CDC vers PC via ST-Link
#define USE_LOGFIFO		// fifo forwardable vers transcript et/ou CDC
#define USE_TRANSCRIPT		// scrollable transcript zone, necessite LOGFIFO
// #define USE_DEMO		// demo des fonts
#define USE_TIME_DATE		// affichage de l'heure
// #define USE_PARAM		// demo de page de parametres ajustables
// #define USE_AUDIO
#define USE_SDCARD		// support SD-Card (FAT32 FS)
// #define USE_UART6	// CN4.D1 = PC6 = UART6 TX, CN4.D0 = PC7 = UART6 RX

// lateralite
#define LEFT_FIX

/** fonts monospace pre-flashees ou non **/

// utiliser option COMPILE_THE_FONTS si les fontes ne sont pas deja flashees
// les fonts seront logees dans le secteur 0 avec l'appli
// N.B. KEIL : pour avoir assez d'espace dans le secteur 0,  enlever USE_TRANSCRIPT dans le projet

// #define COMPILE_THE_FONTS

// utiliser option FLASH_THE_FONTS (qui implique la precedente) pour flasher les fonts
// les fonts seront logees dans le secteur FLASH_FONTS_SECTOR @ FLASH_FONTS_BASE

// #define FLASH_THE_FONTS

// sans ces options les fonts sont supposees presentes @ FLASH_FONTS_BASE

/** modules utilitaires **/
// #define PROFILER_PI2	// pins PI1 aka D13, PI2 aka D8

