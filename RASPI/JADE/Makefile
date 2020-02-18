# listes
SOURCESC=
SOURCESCPP= curlchunk.cpp xmlpc.cpp jade.cpp ssplit2.cpp cli.cpp
EXE= jade.exe
OBJS= $(SOURCESC:.c=.o) $(SOURCESCPP:.cpp=.o)
     
HEADERS= curlchunk.h pcreux.h jade.h ssplit2.h version.h xmlpc.h

INCS=

# LIBS= -lcurldll -lpcre
LIBS= -lcurl -lpcre

# cibles

ALL : $(OBJS) 
	g++ -o $(EXE) $(OBJS) $(LIBS)

clean : 
	rm *.o

delo :
	del *.o

.cpp.o:
	g++ -Wall -Wno-misleading-indentation $(INCS) -c -O3 $<

.c.o:
	gcc -Wall -Wno-misleading-indentation $(INCS) -c -O3 $<

# dependances

cli.o : ${HEADERS}
curlchunk.o : ${HEADERS}
xmlpc.o : ${HEADERS}
ssplit2.o : ssplit2.h
jade.o : ${HEADERS}
