CPP=g++
TARGET=ldmutil
SRC=main.cpp ldm_db.cpp diskio.cpp ldm_parse.cpp ptypenames.cpp
OBJ=$(SRC:%.cpp=%.o)
FLAGS=-O3
LDFLAGS=-Xlinker --strip-all
LIB=
INC=
DEF=-D_GNU_SOURCE -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET=64

.SUFFIXES: .cpp
%.o: %.cpp
	$(CPP) $(DEF) $(INC) $(FLAGS) -c $< -o $@

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CPP) -o $@ $(LDFLAGS) $(OBJ) $(LIB)

$(TARGET)-static: $(OBJ)
	$(CPP) -o $@ --static $(LDFLAGS) $(OBJ) $(LIB)

clean:
	$(RM) *.o $(TARGET) $(TARGET)-static

distclean:	clean

