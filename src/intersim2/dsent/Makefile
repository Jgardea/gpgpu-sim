
# Define the directories that will be compiled
DIRS_TO_COMPILE := util tech \
                   model timing_graph \
                   std_cells \
                   electrical \
                   libutil

DIRS = $(patsubst %,$(CURDIR)/%,$(DIRS_TO_COMPILE))
SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))
OBJS = $(SRCS:%.cc=%.o)

DEF_FLAGS = 
INCLUDE_FLAGS = -I$(CURDIR)
OPT_FLAGS = -O2 -g 
WARN_FLAGS = -Wall 
CXXFLAGS = $(OPT_FLAGS) $(WARN_FLAGS) $(INCLUDE_FLAGS) $(DEF_FLAGS)

LD_LIBS += -lutil
LD_FLAGS += -Llibutil

# Other libraries used 
LIB_UTIL = libutil/libutil.a

#TARGET = $(CURDIR)/libdsent.a
TARGET = $(CURDIR)/dsent
all: $(TARGET)

#$(TARGET): $(OBJS)
#	ar rcs $@ $^

$(TARGET): main.o DSENT.o  $(OBJS) 
	$(CXX) $(CXXFLAGS) $(OBJS) main.o DSENT.o -o $(TARGET) 

# For general c++ compilation
%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

%/created:
	mkdir -p $(dir $@)
	touch $@

.phony: clean
clean:
	$(RM) -rf main.o DSENT.o $(OBJS) $(TARGET)
	$(MAKE) -C $(CURDIR)/libutil clean
