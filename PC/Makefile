# makefile pour MinGW

CCOPT = -Wall -O2
C_SRC = bmp_.c model.c appli.c win32_appli.c
CPP_SRC =
EXE = appli.exe

OBJS = $(C_SRC:.c=.o) $(CPP_SRC:.cpp=.o)

# linkage
$(EXE) : $(OBJS)
	g++ -o $(EXE) $(OBJS) -lgdi32 -lopengl32

# compilage
.c.o :
	gcc $(CCOPT) -c $<

.cpp.o :
	g++ $(CCOPT) -c $<
# other

clean :
	rm *.o; rm *.exe

# dependances : 
