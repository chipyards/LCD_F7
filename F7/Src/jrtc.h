
typedef struct {
int wd;		// weekday monday = 0
int ss;
int mn;
int hh;
int md;		// jour dans le mois 1..31
int mm;		// mois 1..12
// int yy;	// annee 00 -> 99
int day_seconds;
} DAY_TIME;

// mettre la RTC a l'heure a partir de notre structure a nous
void jrtc_set_day_time( DAY_TIME * d );

void jrtc_get_day_time( DAY_TIME * d );

void jrtc_init( void );

// indique si nous avons un demarrage a froid
int jrtc_is_cold_poweron( void );
