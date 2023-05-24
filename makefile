
CFLAGS = -c -fno-asynchronous-unwind-tables -std=gnu++14 -Wall -Wextra
IFLAGS = -I../StdFuncs -D__USE_INLINE__
LFLAGS = -L../StdFuncs/$(OBJ)
LIBS = -lStdFuncs

ifdef PREFIX
	AR = @$(PREFIX)ar
	CC = @$(PREFIX)g++
	LD = @$(PREFIX)g++
	STRIP = @$(PREFIX)strip

	LIBS += -lauto
	STRIP_FLAGS = -R.comment
else
	AR = @ar
	CC = @g++
	LD = @g++
	STRIP = @strip
endif

ifdef DEBUG
	OBJ = Debug
	CFLAGS += -ggdb -D_DEBUG -DMUNGWALL_NO_LINE_TRACKING
else
	OBJ = Release
	CFLAGS += -O2
endif

ifdef PREFIX
	ifeq ($(PREFIX), ppc-amigaos-)
		CFLAGS += -athread=native
		LFLAGS += -athread=native
		OBJ := $(OBJ)_OS4
	else
		IFLAGS += -mcrt=clib2
		LFLAGS += -mcrt=clib2
		LIBS += -lnet
	endif
endif

EXECUTABLE = $(OBJ)/RADRunner

OBJECTS = $(OBJ)/ClientCommands.o $(OBJ)/Execute.o $(OBJ)/RADRunner.o $(OBJ)/ServerCommands.o

all: $(OBJ) $(EXECUTABLE)

$(OBJ):
	@mkdir $(OBJ)

$(EXECUTABLE): $(OBJECTS) ../StdFuncs/$(OBJ)/libStdFuncs.a
	@echo Linking $@...
	$(LD) $(LFLAGS) $(OBJECTS) $(LIBS) -o $@.debug
	$(STRIP) $(STRIP_FLAGS) $@.debug -o $@

$(OBJ)/%.o: %.cpp
	@echo Compiling $<...
	$(CC) $(CFLAGS) $(IFLAGS) -o $(OBJ)/$*.o $<

clean:
	@rm -fr $(OBJ)
