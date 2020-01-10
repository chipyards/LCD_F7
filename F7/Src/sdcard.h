int SDCard_mount(void);
int SDCard_read_test( const char *fnam, char * tbuf, unsigned int size );
int SDCard_write_test( const char *fnam, char * tbuf, unsigned int size );
int SDCard_append_test( const char *fnam, char * tbuf, unsigned int size );
int SDCard_random_write_test( unsigned int size, const char * path, unsigned int * crc );
int SDCard_random_read_test( const char * path, unsigned int * crc, unsigned int * cnt );
