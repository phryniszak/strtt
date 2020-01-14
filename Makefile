# https://stackoverflow.com/questions/172587/what-is-the-difference-between-g-and-gcc
CXX      := -g++
# https://stackoverflow.com/questions/1662909/undefined-reference-to-pthread-create-in-linux
CXXFLAGS := -Wall -pthread

#LDFLAGS  := -L/home/pawel/workspace_c/_openocd/src/jtag/drivers/.libs/ -locdjtagdrivers -lusb-1.0 -lpthread -lstdc++
LDFLAGS  :=  -lusb-1.0

ifdef OS	
LDFLAGS_STATIC := \
	-Lc:\msys64\home\Pawel\openocd\src\jtag\drivers\.libs -locdjtagdrivers \
	-Lc:\msys64\home\Pawel\openocd\src\helper\.libs -lhelper \
	-lws2_32
else
LDFLAGS_STATIC := -L/home/pawel/workspace_c/_openocd/src/jtag/drivers/.libs -locdjtagdrivers
endif

BUILD    := ./build
OBJ_DIR  := $(BUILD)/objects
APP_DIR  := $(BUILD)/apps
TARGET   := app.exe
INCLUDE  := -Iinc/
SRC      :=                      \
   $(wildcard src/*.cpp)         \

OBJECTS := $(SRC:%.cpp=$(OBJ_DIR)/%.o)

all: build $(APP_DIR)/$(TARGET)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ -c $<

$(APP_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $(APP_DIR)/$(TARGET) $(INCLUDE) $(OBJECTS) $(LDFLAGS_STATIC) $(LDFLAGS)

.PHONY: all build clean debug release

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

debug: CXXFLAGS += -DDEBUG -g
debug: all

release: CXXFLAGS += -O2 # -O3
release: all

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/* 

rebuild:
	$(MAKE) clean
	$(MAKE) debug
