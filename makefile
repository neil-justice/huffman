CFLAGS = `sdl2-config --cflags` -O2 -Wall -Wextra -Wfloat-equal -pedantic -ansi -lm
INCS = neillsdl2.h
TARGET = huffsdl
SOURCES =  neillsdl2.c $(TARGET).c
LIBS =  `sdl2-config --libs`
CC = gcc


all: $(TARGET)

$(TARGET): $(SOURCES) $(INCS)
	$(CC) $(SOURCES) -o $(TARGET) $(CFLAGS) $(LIBS)

run: all
	$(TARGET)
