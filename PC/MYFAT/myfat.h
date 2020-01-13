

typedef struct {
int bps; /* bytes par secteur */
int RootDirSectors;
int SectorsPerCluster;
int SectorsPerFAT;
int FirstFATSector;
int FirstDirSector;  /* FAT12/16 only */
int FirstDataSector;
int RootStartCluster; /* FAT32 only */
int TotalSectors;
int DataSectors;
int DataClusters;
int BytesPerFAT;
int FAT; /* 12, 16, ou 32 */
int FATpartBase;  /* abs. premier secteur partition FAT */
int EXTpartBase;  /* abs. secteur partition etendue courante */ 
int EXTpartBaseF; /* abs. secteur partition englobante */ 
unsigned char * fatbuf;
unsigned char * rootbuf;
} MYFAT;

extern MYFAT zemyfat;
#define myfat (&zemyfat)

/** fonctions travaillant en RAM uniquement -------------------------------- */

int DumpPartTable( unsigned char * buf );
int DumpBootSector( unsigned char * buf );
void DumpDirectory( unsigned char * buf, int entries );
// dump hexa+ascii
void HAdump( unsigned char * buf, int q );

// cette fonction rend l'indice de l'entree trouvee (sinon -1), son numero de 1er cluster et sa taille en bytes
int FindFile( char * shortName, unsigned char * dirbuf, int entries, unsigned int * clu, unsigned int * size );
// cette fontion rend le nombre de clusters de la chaine
int ScanChain( unsigned int startCluster );
// statistiques diverses sur la FAT32
int FAT32stat( unsigned int * lastused, unsigned int * freecnt, unsigned int * badcnt, unsigned int * EOCcnt );
// cette fonction convertit la FAT12 en FAT16
void ConvertFAT();

/** fonctions utilisant les fonctions d'acces aux secteurs (diskio) --------------------------- */

// ouvrir le device et decouvrir la FAT, rend zero si ok
int myfat_start_session( const char * devicename );

// alloc et lecture FAT entiere en RAM, retour 0 si Ok
int myfat_read_fat();	// appeler apres succes DumpBootSector()

// alloc et lecture du root dir en RAM, retour 0 si Ok
int myfat_read_root_dir();

// cette fontion alloue charge en memoire les secteurs d'une chaine
int CopyChain( int startCluster, int nclu, unsigned char ** pbuf );

// lecture et copie locale d'une chaine (size en bytes)
int myfat_save_chain( unsigned int startCluster, unsigned int size, const char * local_path );

// lecture et copie d'un paquet de secteurs bruts
int myfat_save_raw( unsigned int startsec, unsigned int qsec, const char * local_path );
