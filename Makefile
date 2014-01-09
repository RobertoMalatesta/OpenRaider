###############################################################
# Mongoose <stu7440@westga.edu>
###############################################################
# + Cleaner clean
# + Add new include tree
# + Add new flags
# + Install/Uninstall
# + Debian and Redhat packaging
# + Lots of misc new features
###############################################################
BUILD_SELECT=debug

NAME=OpenRaider
NAME_TAR=openraider
MAJOR_VERSION=0
MINOR_VERSION=1
MICRO_VERSION=2
BUILD_ID=$(shell date "+%Y%m%d")
PRE=
VERSION=$(MAJOR_VERSION).$(MINOR_VERSION).$(MICRO_VERSION)$(PRE)
VERSION_DEB=$(MAJOR_VERSION).$(MINOR_VERSION).$(MICRO_VERSION).$(BUILD_ID)
BUILD_HOST=$(shell uname -s -n -r -m)
ARCH=$(shell uname -m -s | sed -e "s/ /-/g")
UNAME=$(shell uname -s)

###############################################################

# -DMULTITEXTURE			Add OpenGL multitexturing
# -DUNICODE_SUPPORT			Add unicode/internation keyboard support
# -DUSING_EMITTER_IN_GAME	Run particle test in game

BASE_DEFS=$(shell sdl-config --cflags) -Iinclude -DSDL_INTERFACE \
	-DUSING_OPENGL -DZLIB_SUPPORT -DUSING_EMITTER \
	-DUSING_OPENAL -DUSING_MTK_TGA -DUSING_PTHREADS \
	-DUSING_HEL -DHAVE_SDL_TTF

BASE_LIBS=$(shell sdl-config --libs) -lz -lstdc++ \
	-lpthread -lSDL_ttf

# -DDEBUG_GL
DEBUG_DEFS=-DDEBUG -DEXPERIMENTAL
DEBUG_OBJ=

ifeq ($(UNAME),Darwin)
AUDIO_LIBS += -lalut
AUDIO_LIBS += -framework OpenAL
AUDIO_LIBS += -L/usr/local/lib
AUDIO_DEFS += -I/usr/local/include
GL_LIBS += -framework OpenGL
GL_LIBS += -L/opt/local/lib
GL_DEFS += -I/opt/local/include
else
AUDIO_LIBS += -lopenal
GL_LIBS += -lGL -lGLU
GL_LIBS += -L/usr/local/lib
GL_DEFS += -I/usr/local/include
endif

BASE_LIBS += $(AUDIO_LIBS)
BASE_LIBS += $(GL_LIBS)
BASE_DEFS += $(AUDIO_DEFS)
BASE_DEFS += $(GL_DEFS)

# libferit, File transfer via HTTP/FTP/etc support
LIBFERIT_LIB=/usr/local/lib/libferit.so
LIBFERIT=$(shell if test -e $(LIBFERIT_LIB) > /dev/null; then echo yes; fi)

ifeq ($(LIBFERIT), yes)
	BASE_DEFS += -DHAVE_LIBFERIT
	BASE_LIBS += -lferit
endif

###############################################################

TREE_DIR=OpenRaider
BUILD_DEBUG_DIR=bin/debug
BUILD_RELEASE_DIR=bin/release
BUILD_PROF_DIR=bin/prof
BUILD_TEST_DIR=bin/test
BUILD_MEM_DIR=bin/mem
BUILD_INSTALL_DIR=bin/$(BUILD_SELECT)
DEB_DIR=/tmp/$(NAME).deb

# Edited for Debian GNU/Linux.
DESTDIR =
INSTALL_BIN=$(DESTDIR)/usr/games
INSTALL_LIB=$(DESTDIR)/usr/lib
INSTALL_DOC=$(DESTDIR)/usr/share/doc/$(NAME)
INSTALL_SHARE=$(DESTDIR)/usr/share/$(NAME)
INSTALL_INCLUDE=$(DESTDIR)/usr/include

###############################################################
CC=gcc

BASE_CFLAGS=-Wall $(BASE_DEFS) \
	-DVERSION=\"\\\"$(NAME)-$(VERSION)-$(BUILD_ID)\\\"\" \
	-DBUILD_HOST=\"\\\"$(BUILD_HOST)\\\"\"

LD_FLAGS=-L/usr/X11/lib -lXmu -lXt -lSM -lICE -lXext -lX11 -lXi \
	 -lm $(BASE_LIBS)

RELEASE_CFLAGS=$(BASE_CFLAGS) -ffast-math -funroll-loops \
	-fomit-frame-pointer -O2

DEBUG_CFLAGS=$(BASE_CFLAGS) -g -O0 $(DEBUG_DEFS)

################################################################

DO_CC=$(CC) $(CFLAGS) -o $@ -c $<
DO_SHLIB_CC=$(CC) $(CFLAGS) $(SHLIBCFLAGS) -o $@ -c $<

TARGETS=$(BUILDDIR)/$(NAME)

################################################################
auto: $(BUILD_SELECT)

run: $(BUILD_SELECT)
	bin/$(BUILD_SELECT)/OpenRaider

targets: $(TARGETS)

bundle: release
	mac_dist/bundle.sh
	mac_dist/plist.sh $(NAME) $(VERSION) $(BUILD_ID) > bin/OpenRaider.app/Contents/Info.plist
	mac_dist/frameworks.sh

bundle-image: bundle
	mac_dist/image.sh

bundle-archive: bundle
	mac_dist/archive.sh

all: debug release prof

debug:
	@-mkdir -p $(BUILD_DEBUG_DIR)
	$(MAKE) targets BUILDDIR=$(BUILD_DEBUG_DIR) \
	CFLAGS="$(DEBUG_CFLAGS)" \
	LD_FLAGS="$(LD_FLAGS)"

prof:
	@-mkdir -p $(BUILD_PROF_DIR)
	$(MAKE) targets BUILDDIR=$(BUILD_PROF_DIR) \
	CFLAGS="$(DEBUG_CFLAGS) -pg" \
	LD_FLAGS="$(LD_FLAGS) -pg"

release:
	@-mkdir -p $(BUILD_RELEASE_DIR)
	$(MAKE) targets BUILDDIR=$(BUILD_RELEASE_DIR) \
	CFLAGS="$(RELEASE_CFLAGS)" \
	LD_FLAGS="$(LD_FLAGS)"

#####################################

ded:
	@-mkdir -p $(BUILD_DEBUG_DIR)/ded
	$(MAKE) targets BUILDDIR=$(BUILD_DEBUG_DIR)/ded \
	CFLAGS="$(DEBUG_CFLAGS) -DDEDICATED_SERVER" \
	LD_FLAGS="$(LD_FLAGS)"

# -DDEBUG_MEMORY_VERBOSE
# -DDEBUG_MEMORY
memory:
	@-mkdir -p $(BUILD_MEM_DIR)
	$(MAKE) targets BUILDDIR=$(BUILD_MEM_DIR) \
	DEBUG_OBJ="$(BUILD_MEM_DIR)/memory_test.o" \
	CFLAGS="$(DEBUG_CFLAGS) -DDEBUG_MEMORY" \
	LD_FLAGS="$(LD_FLAGS)"

depend:
	@-echo "Making deps..."
	@-echo "# Autogenerated dependency file" > depend
	@-find ./src -name "*.cpp" -exec ./deps.sh $(BASE_DEFS) {} \; >> depend
	@-echo "[DONE]"

################################################################

# Later hel will become a seperate library once it matures
HEL_OBJ = \
	$(BUILDDIR)/math.o \
	$(BUILDDIR)/Matrix.o \
	$(BUILDDIR)/Quaternion.o \
	$(BUILDDIR)/Vector3d.o \
	$(BUILDDIR)/ViewVolume.o

OBJS = \
	$(DEBUG_OBJ) \
	$(HEL_OBJ) \
	$(BUILDDIR)/Camera.o \
	$(BUILDDIR)/Emitter.o \
	$(BUILDDIR)/GLString.o \
	$(BUILDDIR)/Light.o \
	$(BUILDDIR)/mtk_tga.o \
	$(BUILDDIR)/Network.o \
	$(BUILDDIR)/OpenGLMesh.o \
	$(BUILDDIR)/OpenRaider.o \
	$(BUILDDIR)/Particle.o \
	$(BUILDDIR)/Render.o \
	$(BUILDDIR)/SDLSystem.o \
	$(BUILDDIR)/SkeletalModel.o \
	$(BUILDDIR)/Sound.o \
	$(BUILDDIR)/System.o \
	$(BUILDDIR)/Texture.o \
	$(BUILDDIR)/TombRaider.o \
	$(BUILDDIR)/World.o


$(BUILDDIR)/$(NAME) : $(OBJS)
	$(CC) $(CFLAGS) $(LD_FLAGS) -o $@ $(OBJS)

#################################################################

clean: clean-small clean-dep

clean-small: clean-build clean-test clean-obj 
	@-rm -rf bin/OpenRaider.app
	@-rm -rf bin/OpenRaider.dmg
	@-rm -rf bin/OpenRaider.zip

clean-dep:
	@-echo "Cleaning dependencies"
	@-rm -f depend
	@-echo "[DONE]"

clean-test:
	@-echo "Cleaning test builds"
	@-rm -f $(BUILD_TEST_DIR)/*.o
	@-rm -f $(BUILD_TEST_DIR)/*.test
	@-rm -rf $(BUILD_TEST_DIR)/*.build
	@-echo "[DONE]"

clean-obj:
	@-echo "Cleaning objects"
	@-rm -f $(BUILD_PROF_DIR)/*.o
	@-rm -f $(BUILD_DEBUG_DIR)/*.o
	@-rm -f $(BUILD_RELEASE_DIR)/*.o
	@-rm -f $(BUILD_TEST_DIR)/*.o
	@-rm -f $(BUILD_MEM_DIR)/*.o
	@-echo "[DONE]"

clean-build:
	@-echo "Cleaning builds"
	@-rm -f $(BUILD_PROF_DIR)/$(NAME)
	@-rm -f $(BUILD_DEBUG_DIR)/$(NAME)
	@-rm -f $(BUILD_RELEASE_DIR)/$(NAME)
	@-rm -f $(BUILD_MEM_DIR)/$(NAME)
	@-echo "[DONE]"


#################################################################

-include depend

#################################################################

ifneq ($(UNAME),Darwin)

install:
	mkdir -p $(INSTALL_SHARE)/data
	cp setup.sh $(INSTALL_SHARE)
	cp data/* $(INSTALL_SHARE)/data
	mkdir -p $(INSTALL_DOC)
	cp README.md README.old ChangeLog
	mkdir -p $(INSTALL_BIN)
	cp bin/$(BUILD_SELECT)/OpenRaider $(INSTALL_BIN)

bin-tarball: clean-build clean-test clean-obj $(BUILD_SELECT)
	@-cd .. && tar zcvf $(NAME_TAR)-$(VERSION_DEB)-$(ARCH).tar.gz \
		$(TREE_DIR)/Makefile $(TREE_DIR)/data \
		$(TREE_DIR)/bin/$(BUILD_SELECT)/OpenRaider \
		$(TREE_DIR)/README.md $(TREE_DIR)/ChangeLog

endif

#################################################################
# Unit Test builds
#################################################################
TEST_FLAGS=-Wall -g -O0 -DDEBUG -lstdc++

TEST_MAP_TR5=~/projects/Data/models/tombraider/tr5/demo.trc
TEST_MAP_TR4=~/projects/Data/models/tombraider/tr4/angkor1.tr4
TEST_MAP_TR3=~/projects/Data/models/tombraider/tr3/scotland.tr2
TEST_MAP_TR2=~/projects/Data/models/tombraider/tr2/unwater.tr2
TEST_MAP_TR1=~/projects/Data/models/tombraider/tr1/level1.phd

TombRaider.reg_test:
	$(MAKE) TombRaider.test
	$(BUILD_TEST_DIR)/TombRaider.test load $(TEST_MAP_TR1) > /tmp/log.tr1
	$(BUILD_TEST_DIR)/TombRaider.test load $(TEST_MAP_TR2) > /tmp/log.tr2
	$(BUILD_TEST_DIR)/TombRaider.test load $(TEST_MAP_TR3) > /tmp/log.tr3
	$(BUILD_TEST_DIR)/TombRaider.test load $(TEST_MAP_TR4) > /tmp/log.tr4
	$(BUILD_TEST_DIR)/TombRaider.test load $(TEST_MAP_TR5) > /tmp/log.tr5


TombRaider.test:
	@-mkdir -p $(BUILD_TEST_DIR)
	$(MAKE) targets NAME=TombRaider.test BUILDDIR=$(BUILD_TEST_DIR) \
	OBJS="$(BUILD_TEST_DIR)/TombRaider.o $(BUILD_TEST_DIR)/mtk_tga.o $(BUILD_TEST_DIR)/memory_test.o" \
	CFLAGS="$(BASE_CFLAGS) -g -D__TOMBRAIDER_TEST__ -D__TEST_TR5_DUMP_TGA -D__TEST_32BIT_TEXTILES -DDEBUG_MEMORY" \
	LD_FLAGS="-lz -lstdc++"

#################################################################

GLString.test:
	mkdir -p $(BUILD_TEST_DIR)
	$(CC) -Wall -Iinclude -D__TEST__ -DHAVE_MTK -DHAVE_SDL -DUSING_MTK_TGA \
	$(shell sdl-config --cflags) $(shell sdl-config --libs) \
	$(GL_LIBS) $(GL_DEFS) -lm -lstdc++ \
	src/Texture.cpp src/mtk_tga.cpp \
	src/GLString.cpp -o $(BUILD_TEST_DIR)/GLString.test

#################################################################

Hel.test: Quaternion.test Matrix.test Math.test

Matrix.test:
	@-echo "Building Matrix unit test"
	mkdir -p $(BUILD_TEST_DIR)
	$(CC) -Wall -g -DMATRIX_UNIT_TEST -lm -lstdc++ -Iinclude \
	src/hel/Matrix.cpp src/hel/Quaternion.cpp src/hel/Vector3d.cpp \
	-o $(BUILD_TEST_DIR)/Matrix.test
	@-echo "================================================="
	@-echo "Running Matrix unit test"
	$(BUILD_TEST_DIR)/Matrix.test

Quaternion.test:
	@-echo "Building Quaternion unit test"
	mkdir -p $(BUILD_TEST_DIR)
	$(CC) -Wall -g -DUNIT_TEST_QUATERNION -lm -lstdc++ -Iinclude \
	src/hel/Quaternion.cpp -o $(BUILD_TEST_DIR)/Quaternion.test
	@-echo "================================================="
	@-echo "Running Quaternion unit test"
	$(BUILD_TEST_DIR)/Quaternion.test

Math.test:
	@-echo "Building Math unit test"
	mkdir -p $(BUILD_TEST_DIR)
	$(CC) -Wall -g -DMATH_UNIT_TEST -lm -lstdc++ -Iinclude \
	src/hel/math.cpp src/hel/Vector3d.cpp -o $(BUILD_TEST_DIR)/Math.test
	@-echo "================================================="
	@-echo "Running hel unit test"
	$(BUILD_TEST_DIR)/Math.test

#################################################################

Memory.test:
	mkdir -p $(BUILD_TEST_DIR)
	$(CC) -Wall -g -lstdc++ -Iinclude \
	-DDEBUG_MEMORY -DDEBUG_MEMORY_ERROR \
	src/memory_test.cpp test/memory_test.cpp -o $(BUILD_TEST_DIR)/memory_test.test

#################################################################

Network.test:
	mkdir -p $(BUILD_TEST_DIR)
	$(CC) $(TEST_FLAGS) -DUNIT_TEST_NETWORK \
	src/Network.cpp -o $(BUILD_TEST_DIR)/Network.test

#################################################################

Sound.test:
	mkdir -p $(BUILD_TEST_DIR)
	$(CC) $(TEST_FLAGS) -DUNIT_TEST_SOUND \
		-DUSING_OPENAL $(AUDIO_LIBS) \
		src/Sound.cpp -o $(BUILD_TEST_DIR)/Sound.test
ifeq ($(UNAME),Darwin)
	install_name_tool -change libalut.0.1.0.dylib /opt/local/lib/libalut.0.1.0.dylib $(BUILD_TEST_DIR)/Sound.test
endif

#################################################################

TGA.test:
	mkdir -p $(BUILD_TEST_DIR)
	$(CC) $(TEST_FLAGS) -DUNIT_TEST_TGA \
		src/mtk_tga.cpp -o $(BUILD_TEST_DIR)/TGA.test

#################################################################
