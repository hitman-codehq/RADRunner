
CFLAGS = -c -fno-asynchronous-unwind-tables -std=c++14 -Wall -Wextra -Wwrite-strings -DMUNGWALL_NO_LINE_TRACKING
IFLAGS = -I../StdFuncs -D__USE_INLINE__
LFLAGS = -L../StdFuncs/$(OBJ)
LIBS = -lStdFuncs

ifdef PREFIX
	AR = @$(PREFIX)ar
	CC = @$(PREFIX)g++
	LD = @$(PREFIX)g++
	STRIP = @$(PREFIX)strip
else
	AR = @ar
	CC = @g++
	LD = @g++
	STRIP = @strip
	IFLAGS := -ISDK:NDK_3.9/Include/include_h -ISDK:AHI/Include
endif

ifdef DEBUG
	OBJ = Debug
	CFLAGS += -ggdb -D_DEBUG
else
	OBJ = Release
	CFLAGS += -O2
endif

UNAME = $(shell uname)

ifeq ($(UNAME), CYGWIN_NT-10.0)

CFLAGS += -athread=native
LFLAGS += -athread=native
# TODO: CAW - Fix or remove this
#LIBS += -lauto

else

LFLAGS += -mcrt=clib2
LIBS += -lnet

endif

EXECUTABLE = $(OBJ)/RADRunner

OBJECTS = $(OBJ)/ClientCommands.o $(OBJ)/RADRunner.o $(OBJ)/ServerCommands.o $(OBJ)/StdSocket.o

All: $(OBJ) $(EXECUTABLE)

$(OBJ):
	@mkdir $(OBJ)

$(EXECUTABLE): $(OBJECTS) ../StdFuncs/$(OBJ)/libStdFuncs.a
	@echo Linking $@...
	$(LD) $(OBJECTS) $(LFLAGS) $(LIBS) -o $@.debug
	$(STRIP) -R.comment $@.debug -o $@

$(OBJ)/%.o: %.cpp
	@echo Compiling $<...
	$(CC) $(CFLAGS) $(IFLAGS) -o $(OBJ)/$*.o $<

clean:
	@rm -fr $(OBJ)
