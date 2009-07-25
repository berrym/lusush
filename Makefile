CC=gcc
CFLAGS=-Wall -lreadline
OBJDIR=obj
SRCDIR=src
SRCS=$(SRCDIR)/builtins.c \
        $(SRCDIR)/cmdlist.c \
        $(SRCDIR)/env.c \
        $(SRCDIR)/exec.c \
        $(SRCDIR)/init.c \
        $(SRCDIR)/input.c \
        $(SRCDIR)/misc.c \
        $(SRCDIR)/lusush.c \
        $(SRCDIR)/parse.c
OBJS=$(OBJDIR)/builtins.o \
        $(OBJDIR)/cmdlist.o \
        $(OBJDIR)/env.o \
        $(OBJDIR)/exec.o \
		$(OBJDIR)/init.o \
        $(OBJDIR)/input.o \
		$(OBJDIR)/misc.o \
		$(OBJDIR)/lusush.o \
        $(OBJDIR)/parse.o
EXE=lusush
MV=mv
DEL=rm -rf
all:
	$(CC) $(CFLAGS) -c $(SRCS)
	$(MV) *.o $(OBJDIR)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXE)
debug:
	$(CC) $(CFLAGS) -g -c $(SRCS)
	$(MV) *.o $(OBJDIR)
	$(CC) $(CFLAGS) -g $(OBJS) -o $(EXE)
clean:
	$(DEL) $(OBJS)
	$(DEL) $(EXE)
