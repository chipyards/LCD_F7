# makefile pour MinGW

CCOPT = -Wall -O2
C_SRC = diskio.c myfat.c appli.c
CPP_SRC =
EXE = appli

OBJS = $(C_SRC:.c=.o) $(CPP_SRC:.cpp=.o)

# linkage
$(EXE) : $(OBJS)
	g++ -o $(EXE) $(OBJS)

# compilage
.c.o :
	gcc $(CCOPT) -c $<

.cpp.o :
	g++ $(CCOPT) -c $<
# other

clean :
	rm *.o; rm *.exe

# dependances : 
diskio.o : diskio.h
myfat.o : diskio.h myfat.h
appli.o : diskio.h myfat.h
