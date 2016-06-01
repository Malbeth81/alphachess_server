# Global parameters
GCC = g++.exe
#FLAGS = -Wall -fmessage-length=0 -I..\includes -g -DDEBUG
FLAGS = -Wall -fmessage-length=0 -I..\includes -O2
TARGET = bin\alphachessserver.exe

all: $(TARGET)

# Create target application
$(TARGET): res\resources.res obj\main.o obj\alphachessserver.o obj\gameserverclient.o obj\gameserver.o
	$(GCC) -static-libgcc -static-libstdc++ -mwindows -o $@ $^ -l ws2_32

#Resources
res\resources.res: res\resources.rc
	windres.exe --input-format=rc -O coff -o $@ -i $<

#Root
obj\main.o: src\main.cpp
	$(GCC) $(FLAGS) -o $@ -c $<

obj\alphachessserver.o: src\alphachessserver.cpp src\alphachessserver.h
	$(GCC) $(FLAGS) -o $@ -c $<

obj\gameserverclient.o: src\gameserverclient.cpp src\gameserverclient.h
	$(GCC) $(FLAGS) -o $@ -c $<

obj\gameserver.o: src\gameserver.cpp src\gameserver.h
	$(GCC) $(FLAGS) -o $@ -c $<

# Clean targets
clean:
	del obj\*.o
	del $(TARGET)