# listes
SOURCESC= bcm2835.c spix.c
SOURCESCPP= 
EXE= spix.exe
OBJS= $(SOURCESC:.c=.o) $(SOURCESCPP:.cpp=.o)
     
HEADERS= bcm2835.h

INCS=

LIBS= 
# cibles

ALL : $(OBJS) 
	g++ -o $(EXE) $(OBJS) $(LIBS)

clean : 
	rm *.o

delo :
	del *.o

.cpp.o:
	g++ -Wall -Wno-misleading-indentation $(INCS) -c -O2 $<

.c.o:
	gcc -Wall -Wno-misleading-indentation $(INCS) -c -O2 $<

# dependances

bcm2835.o : ${HEADERS}
spix.o : ${HEADERS}
