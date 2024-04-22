CXX := clang++
CXXFLAGS := -std=c++17 -g -Wall -O3
INCLUDES := -I/usr/include/SDL2 -I./ThirdParty -I./ThirdParty/box2d -I./ThirdParty/glm-0.9.9.8 -I./ThirdParty/LuaBridge -I./ThirdParty/rapidjson
LDFLAGS := -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -llua5.4

# Paths to object files for Box2D
BOX2D_SRCS := $(wildcard ./ThirdParty/box2d/collision/*.cpp) \
               $(wildcard ./ThirdParty/box2d/common/*.cpp) \
               $(wildcard ./ThirdParty/box2d/dynamics/*.cpp) \
               $(wildcard ./ThirdParty/box2d/rope/*.cpp)

# Auto-detect all cpp files for the main project and Box2D
PROJECT_SRCS := $(wildcard *.cpp)
ALL_SRCS := $(PROJECT_SRCS) $(BOX2D_SRCS)

OBJS := $(ALL_SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)

# Name of the executable to create
MAIN := game_engine_linux

# Default target
all: $(MAIN)

# Link the executable from the object files
$(MAIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Include dependency rules
-include $(DEPS)

# Compile the object files from cpp files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -c $< -o $@

# Phony target for cleaning the build
.PHONY: clean
clean:
	$(RM) $(OBJS) $(DEPS) $(MAIN)

# Phony target for a full clean (including any saved session data)
.PHONY: distclean
distclean: clean
	$(RM) *~ *.swp

# Phony target for running the game engine
.PHONY: run
run: all
	./$(MAIN)
