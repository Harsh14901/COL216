CXX = g++
CXXFLAGS	:= -g -std=c++17  -fno-stack-protector
LFLAGS =
OUTPUT	:= output
BUILD 			:= build
SRC		:= src
INCLUDE	:= include
LIB		:= lib


MAIN	:= main
SOURCEDIRS	:= $(shell find $(SRC) -type d)
INCLUDEDIRS	:= $(shell find $(INCLUDE) -type d)
LIBDIRS		:= $(shell find $(LIB) -type d)
FIXPATH = $1
RM = rm -f
MD	:= mkdir -p

INCLUDES	:= $(patsubst %,-I%, $(INCLUDEDIRS:%/=%))
LIBS		:= $(patsubst %,-L%, $(LIBDIRS:%/=%))
SOURCES		:= $(wildcard $(patsubst %,%/*.cpp, $(SOURCEDIRS)))
OBJECTS		:= $(addprefix $(BUILD)/, $(SOURCES:.cpp=.o))

OUTPUTMAIN	:= $(call FIXPATH,$(OUTPUT)/$(MAIN))

args := input/file

all: $(OUTPUT) $(BUILD)/$(SOURCEDIRS) $(MAIN)
	@echo Executing 'all' complete!

$(OUTPUT):
	$(MD) $(OUTPUT)

$(BUILD)/$(SOURCEDIRS):
	$(MD) $(BUILD)/$(SOURCEDIRS)

$(MAIN): $(OBJECTS) 
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LFLAGS) -o $(OUTPUTMAIN) $(OBJECTS)

$(BUILD)/%.o: %.cpp 
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@ 

.PHONY: clean
clean:
	$(RM) $(OUTPUTMAIN)
	$(RM) $(call FIXPATH,$(OBJECTS))
	@echo Cleanup complete!

run: all
	./$(OUTPUTMAIN) ${args}
	@echo Executing 'run: all' complete!

zip:
	zip 2019CS10431_2019CS50447.zip src/ include/ doc/Documentation.pdf ./Makefile -r tests/