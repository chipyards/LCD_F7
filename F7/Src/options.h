#define VERSION "3.5 keil"

// #define GREEN_CPU		// sleep dans main loop

/* modules optionnels du display */
#define USE_TRANSCRIPT		// scrollable transcript zone
#define USE_DEMO		// demo des fonts
#define USE_TIME_DATE		// affichage de l'heure
#define USE_UART1		// CDC vers PC via ST-Link
#define USE_PARAM		// page de parametres ajustables
// #define USE_UART6	// CN4.D1 = PC6 = UART6 TX, CN4.D0 = PC7 = UART6 RX

// lateralite
#define LEFT_FIX

// modules utilitaires
// #define PROFILER_PI2	// pins PI1 aka D13, PI2 aka D8
// #define COMPILE_THE_FONTS
// #define FLASH_THE_FONTS	// seulement 1 fois, sur carte neuve

