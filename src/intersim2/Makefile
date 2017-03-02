# $Id $

# Copyright (c) 2007-2012, Trustees of The Leland Stanford Junior University
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

#
# Makefile
#
CXX = g++
CC = gcc
CREATE_LIBRARY ?= 0
INTERFACE = interconnect_interface.cpp gputrafficmanager.cpp
DEBUG ?= 0

USE_DSENT = -DALLOW_DSENT

DSENT := dsent

LEX = flex
YACC   = bison -y
DEFINE = #-DTRACK_STALLS -DTRACK_BUFFERS -DTRACK_FLOWS -DTRACK_CREDITS
INCPATH = -I. -Iarbiters -Iallocators -Irouters -Inetworks -Ipower -Idsent -Idsent/libutil


ifeq ($(CREATE_LIBRARY),1)
INCPATH += -I$(GPGPUSIM_ROOT)/src/gpgpu-sim/
endif

CPPFLAGS += -Wall $(INCPATH) $(DEFINE)
ifneq ($(DEBUG),1)
CPPFLAGS += -O3
endif
CPPFLAGS += -g
CPPFLAGS += -fPIC

ifeq ($(SIM_OBJ_FILES_DIR),)
OBJDIR := obj
else
OBJDIR := $(SIM_OBJ_FILES_DIR)/intersim2
endif
PROG   := booksim

# simulator source files
CPP_SRCS =  \
   config_utils.cpp \
   booksim_config.cpp \
   module.cpp \
   buffer.cpp \
   vc.cpp \
   routefunc.cpp \
   traffic.cpp \
   flitchannel.cpp \
   trafficmanager.cpp \
   batchtrafficmanager.cpp \
   packet_reply_info.cpp \
   buffer_state.cpp \
   stats.cpp \
   credit.cpp \
   outputset.cpp \
   flit.cpp \
   injection.cpp\
   misc_utils.cpp\
   rng_wrapper.cpp\
   rng_double_wrapper.cpp\
   power_module.cpp \
   switch_monitor.cpp \
   buffer_monitor.cpp \
   main.cpp \
   intersim_config.cpp \
   vertical_channel.cpp 

CC_SRCS = $(wildcard dsent/*.cc)

ifeq ($(CREATE_LIBRARY),1)
CPP_SRCS += $(INTERFACE) 
DEFINE += -DCREATE_LIBRARY
else 
DEFINE += -DBOOKSIM
endif

LEX_OBJS  = ${OBJDIR}/lex.yy.o
YACC_OBJS = ${OBJDIR}/y.tab.o

# Booksim Directories 
NETWORKS:= $(wildcard networks/*.cpp) 
ALLOCATORS:= $(wildcard allocators/*.cpp)
ARBITERS:= $(wildcard arbiters/*.cpp)
ROUTERS:= $(wildcard routers/*.cpp)
POWER:= $(wildcard power/*.cpp)

#DSENT Directories
UTIL:= $(wildcard ${DSENT}/util/*.cc)
TECH:=$(wildcard ${DSENT}/tech/*.cc)
MODEL:=$(wildcard ${DSENT}/model/*.cc)
STD_CELLS:=$(wildcard ${DSENT}/std_cells/*.cc)
ELECTRICAL:=$(wildcard ${DSENT}/electrical/*.cc)
TIMING:=$(wildcard ${DSENT}/timing_graph/*.cc)
LIBUTIL:=$(wildcard ${DSENT}/libutil/*.cc)

##--- Make rules ---
OBJS :=  $(LEX_OBJS) $(YACC_OBJS)\
 $(CPP_SRCS:%.cpp=${OBJDIR}/%.o)\
 $(NETWORKS:networks/%.cpp=${OBJDIR}/%.o)\
 $(ALLOCATORS:allocators/%.cpp=${OBJDIR}/%.o)\
 $(ARBITERS:arbiters/%.cpp=${OBJDIR}/%.o)\
 $(ROUTERS:routers/%.cpp=${OBJDIR}/%.o)\
 $(POWER:power/%.cpp=${OBJDIR}/%.o)\

ifeq ($(USE_DSENT),-DALLOW_DSENT)
OBJS += $(CC_SRCS:${DSENT}/%.cc=${OBJDIR}/%.o)\
 $(TECH:${DSENT}/tech/%.cc=${OBJDIR}/%.o)\
 $(MODEL:${DSENT}/model/%.cc=${OBJDIR}/%.o)\
 $(STD_CELLS:${DSENT}/std_cells/%.cc=${OBJDIR}/%.o)\
 $(ELECTRICAL:${DSENT}/electrical/%.cc=${OBJDIR}/%.o)\
 $(TIMING:${DSENT}/timing_graph/%.cc=${OBJDIR}/%.o)\
 $(LIBUTIL:${DSENT}/libutil/%.cc=${OBJDIR}/%.o) \
 $(UTIL:${DSENT}/util/%.cc=${OBJDIR}/%.o)\

DEFINE += $(USE_DSENT)
endif

ifeq ($(CREATE_LIBRARY),1)
all: $(OBJS) 
else
all:$(PROG)

$(PROG): $(OBJS) 
	 $(CXX) $(LFLAGS) $^ -o $@ 
endif

# rules to compile simulator

lex.yy.c: config.l
	$(LEX) $<

y.tab.c y.tab.h: config.y
	$(YACC) -d $<

${LEX_OBJS}: lex.yy.c y.tab.h	
	$(CC) $(CPPFLAGS) -c $< -o $@

${YACC_OBJS}: y.tab.c y.tab.h
	$(CC) $(CPPFLAGS) -c $< -o $@

${OBJDIR}/%.o: %.cpp 
	$(CXX) $(CPPFLAGS) -c $< -o $@

# rules to compile networks
${OBJDIR}/%.o: networks/%.cpp 
	$(CXX) $(CPPFLAGS) -c $< -o $@

# rules to compile arbiters
${OBJDIR}/%.o: arbiters/%.cpp 
	$(CXX) $(CPPFLAGS) -c $< -o $@

# rules to compile allocators
${OBJDIR}/%.o: allocators/%.cpp 
	$(CXX) $(CPPFLAGS) -c $< -o $@

# rules to compile routers
${OBJDIR}/%.o: routers/%.cpp 
	$(CXX) $(CPPFLAGS) -c $< -o $@

# rules to compile power classes
${OBJDIR}/%.o: power/%.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

#DSENT rules to compile

${OBJDIR}/%.o: ${DSENT}/%.cc
	$(CXX) $(CPPFLAGS)  -c $< -o $@

${OBJDIR}/%.o: ${DSENT}/util/%.cc
	$(CXX) $(CPPFLAGS) -c $< -o $@

${OBJDIR}/%.o: ${DSENT}/tech/%.cc
	$(CXX) $(CPPFLAGS) -c $< -o $@

${OBJDIR}/%.o: ${DSENT}/model/%.cc
	$(CXX) $(CPPFLAGS) -c $< -o $@

${OBJDIR}/%.o: ${DSENT}/std_cells/%.cc
	$(CXX) $(CPPFLAGS) -c $< -o $@

${OBJDIR}/%.o: ${DSENT}/electrical/%.cc
	$(CXX) $(CPPFLAGS) -c $< -o $@

${OBJDIR}/%.o: ${DSENT}/timing_graph/%.cc
	$(CXX) $(CPPFLAGS) -c $< -o $@

${OBJDIR}/%.o: ${DSENT}/libutil/%.cc
	$(CXX) $(CPPFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(OBJS) 
	rm -f $(PROG)
	rm -f $(CPP_DEPS)
	rm -f $(CC_DEPS)
	rm -f y.tab.c y.tab.h lex.yy.c
	rm -f moc_bgui.cpp
	
	rm -f *~ */*~
	rm -f *.o */*.o
	rm -f *.d */*.d
	rm -f allocators/*~
	rm -f arbiters/*~
	rm -f networks/*~
	rm -f runfiles/*~
	rm -f routers/*~
	rm -f examples/*~
	rm -f dsent/util/*~
	rm -f dsent/libutil/*~
	rm -f dsent/electrical/*~
	rm -f dsent/tech/*~
	rm -f dsent/model/*~
	rm -f dsent/std_cells/*~
	rm -f dsent/timing_graph/*~

-include $(CPP_DEPS)
-include $(CC_DEPS)
