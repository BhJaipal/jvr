# SPDX-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 BhJaipal
LIBS := glfw3 vulkan

PLATFORM_CONF ?= $(subst build/PLATFORM-,,$(wildcard build/PLATFORM-*))
ifeq ($(PLATFORM_CONF), )
	ifeq ($(OS), Windows_NT)
		PLATFORM_CONF = win
	else
		PLATFORM_CONF = $$XDG_SESSION_TYPE
	endif
endif

ifeq ($(PLATFORM_CONF), win)
else ifeq ($(PLATFORM_CONF), wayland)
	LIBS += wayland-client
else ifeq ($(PLATFORM_CONF), xcb)
	LIBS += x11
else ifeq ($(PLATFORM_CONF), xlib)
	LIBS += x11 xcb
endif

DIR = jvr
SRC := $(wildcard $(DIR)/*.cpp)
OBJ := $(foreach s, $(SRC), $(subst $(DIR)/,build/,$(s:.cpp=.o)))
MAIN ?= donut.cpp
MAINC ?= donut.c
SHADER ?= donut

DIRC = jvrc
SRCC := $(wildcard $(DIRC)/*.c)
OBJC := $(foreach s, $(SRCC), $(subst $(DIRC)/,build/,$(s:.c=c.o)))

PLATFORM ?= $(PLATFORM_CONF)

all: a.out

target:
	@echo "#define VK_PLATFORM_$(PLATFORM)" > vk-platform.h
	@if [ ! -d build ]; then mkdir build; fi
	@touch build/PLATFORM-$(PLATFORM)

list-target:
	@echo "Available targets: win wayland xlib xcb"


build/%.vert.spv: %.vert
	@echo -e "\r    \e[1;95mGLSLC  \e[0m " $@ "\e[0m"
	@glslc $< -o $@

build/%.frag.spv: %.frag
	@echo -e "\r    \e[1;95mGLSLC  \e[0m " $@ "\e[0m"
	@glslc $< -o $@

build/%.o: $(DIR)/%.cpp
	@echo -e "\r    \e[1;92mCXX    \e[0m " $@ "\e[0m"
	@g++ -c -std=c++23 -g $< -o $@ -fPIC

build/%c.o: $(DIRC)/%.c
	@echo -e "\r    \e[1;92mCC     \e[0m " $@ "\e[0m"
	@gcc -c -g $< -o $@ -fPIC

build/jvk.so: $(OBJ)
	@echo -e "\r    \e[1;93mLD     \e[0;1m " $@ "\e[0m"
	@g++ -shared -std=c++23 -g $^ -o $@ -fPIC

build/jvkc.so: $(OBJC)
	@echo -e "\r    \e[1;93mLD     \e[0;1m " $@ "\e[0m"
	@gcc -shared -g $^ -o $@ -fPIC

build/%c.o: %.c $(wildcard $(DIR)/*.h)
	@echo -e "\r    \e[1;92mCC     \e[0m " $@ "\e[0m"
	@gcc -c -g $< -o $@ #-fsanitize=address,undefined

build/%.o: %.cpp $(wildcard $(DIR)/*.hpp)
	@echo -e "\r    \e[1;92mCXX    \e[0m " $@ "\e[0m"
	@g++ -c -std=c++23 -g $< -o $@ #-fsanitize=address,undefined

a.out: build/$(subst .cpp,.o,$(MAIN)) build/jvk.so
	@echo -e "\r    \e[1;93mLD     \e[0;1m " $@ "\e[0m"
	@g++ -g -std=c++23 -O3 $< build/jvk.so $$(pkg-config --libs $(LIBS)) #-fsanitize=address,undefined

ac.out: build/$(subst .c,c.o,$(MAINC)) build/jvkc.so
	@echo -e "\r    \e[1;93mLD     \e[0;1m " $@ "\e[0m"
	@gcc -g -o $@ -O3 $< build/jvkc.so $$(pkg-config --libs $(LIBS)) #-fsanitize=address,undefined

test: a.out build/$(SHADER).vert.spv build/$(SHADER).frag.spv
	@./$<

testc: ac.out build/$(SHADER).vert.spv build/$(SHADER).frag.spv
	@./$<
