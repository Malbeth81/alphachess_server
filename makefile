# Global parameters
GCC = g++.exe
FLAGS =	-Wall -fmessage-length=0 -I..\includes -O2 -DDEBUG
TARGET = bin\alphachessserver.exe

all: $(TARGET)

# Create target application
$(TARGET): obj\main.o obj\gameserver.o
	$(GCC) -mwindows -o $@ $^ -l ws2_32

#Root
obj\main.o: src\main.cpp
	$(GCC) $(FLAGS) -o $@ -c $<

obj\gameserver.o: src\gameserver.cpp src\gameserver.h
	$(GCC) $(FLAGS) -o $@ -c $<

# Clean targets
clean:
	del obj\*.o
	del $(TARGET)