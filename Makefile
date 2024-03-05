.SILENT:
all: build

verbose := 0

GCC_COMPILE_OPTIONS := -std=c++14 -g
# GCC_COMPILE_OPTIONS="-std=c++14 -O3"
GCC_INCLUDE_DIRS := -Iinclude -include include/pch.h -Ilibs/tracy-0.10/public
GCC_DEFINITIONS := -DOS_WINDOWS
# GCC_DEFINITIONS := -DOS_WINDOWS -DTRACY_ENABLE
# GCC_DEFINITIONS := -DOS_UNIX
GCC_WARN := -Wall -Wno-unused-variable -Wno-attributes -Wno-unused-value -Wno-null-dereference -Wno-missing-braces -Wno-unused-private-field -Wno-unknown-warning-option -Wno-unused-but-set-variable -Wno-nonnull-compare 
GCC_WARN := $(GCC_WARN) -Wno-sign-compare 
GCC_LIBS := -lAdvapi32 -lgdi32 -luser32 -lkernel32 -lshell32 -lws2_32 -ldbghelp

# CC := clang++
CC := g++

# compiler executable
OUTPUT := bin/app.exe

SRC := $(wildcard src/*.cpp)
OBJ := $(patsubst src/%.cpp,bin/%.o,$(SRC))
DEP := $(patsubst bin/%.o,bin/%.d,$(OBJ))

-include $(DEP)

bin/%.o: src/%.cpp
	@ rem echo $<
	@ rem mkdir -p bin
	@rem mkdir bin 2> nul
	$(CC) $(GCC_WARN) $(GCC_COMPILE_OPTIONS) $(GCC_INCLUDE_DIRS) $(GCC_DEFINITIONS) -c $< -o $@ -MD -MP

bin/tracy.o:
	$(CC) -c $(GCC_COMPILE_OPTIONS) $(GCC_INCLUDE_DIRS) $(GCC_DEFINITIONS) libs\tracy-0.10\public\TracyClient.cpp -o bin/tracy.o

build: $(OBJ) bin/tracy.o
	@rem echo $(OUTPUT)
	$(CC) -o $(OUTPUT) $(OBJ) $(GCC_LIBS)
	@rem $(CC) -o $(OUTPUT) $(OBJ) bin/tracy.o $(GCC_LIBS)
	@rem cp bin/app.exe app.exe
	echo f | xcopy bin\app.exe app.exe /y /q > nul

clean:
	del /Q bin
	@rem rm -rf bin/*