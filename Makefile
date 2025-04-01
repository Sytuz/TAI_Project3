# Define the compiler and flags
CXX      = g++
CXXFLAGS = -Wall -std=c++17

# List source files and automatically generate object file names
SOURCES_META = MetaClass.cpp FCMModel.cpp utils.cpp
SOURCES_TEST = tests.cpp FCMModel.cpp utils.cpp
OBJECTS_META = $(SOURCES_META:.cpp=.o)
OBJECTS_TEST = $(SOURCES_TEST:.cpp=.o)
TARGET_META  = MetaClass
TARGET_TEST  = tests

# Default target to build both executables
all: $(TARGET_META) $(TARGET_TEST)

# Link object files into the final executables
$(TARGET_META): $(OBJECTS_META)
	$(CXX) $(CXXFLAGS) -o $(TARGET_META) $(OBJECTS_META)

$(TARGET_TEST): $(OBJECTS_TEST)
	$(CXX) $(CXXFLAGS) -o $(TARGET_TEST) $(OBJECTS_TEST)

# Rule for compiling .cpp files into .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(TARGET_META) $(TARGET_TEST) $(OBJECTS_META) $(OBJECTS_TEST)

# Quick test target
run_test: $(TARGET_TEST)
	./$(TARGET_TEST)

# Build only MetaClass
metaclass: $(TARGET_META)

# Build only tests
tests_only: $(TARGET_TEST)

# More verbose help
help:
	@echo "Available targets:"
	@echo "  all        - Build both MetaClass and tests (default)"
	@echo "  metaclass  - Build only MetaClass"
	@echo "  tests_only - Build only tests"
	@echo "  run_test   - Build and run tests with default parameters"
	@echo "  clean      - Remove all built files"
	@echo "  help       - Display this help message"