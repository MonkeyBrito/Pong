# EduardoBrito [12/04/2023] [13:52].

rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

#TODO: check static lib call/export
#TODO: add dll build option

# Project.
APPNAME?=Test

# Binary Files.
EXEFILE?=$(APPNAME).exe

# Binary Directories.
BINDIR?=bin

# Object Directories.
OBJDIR?=obj

# Source Directories.
SRCDIR?=src

# Shader Directory.
SHADIR?=assets/shaders

# Get Source Files.
SOURCES:=$(call rwildcard,$(SRCDIR)/,*.c)
SHA_SOURCES:=$(call rwildcard,$(SHADIR)/,*.vert)
SHA_SOURCES+=$(call rwildcard,$(SHADIR)/,*.frag)
RES_SRC:=$(SRCDIR)/res/resource.rc

# Set Object Files Location.
OBJECTS:=$(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
RES_OBJ:=$(OBJDIR)/res/resource.o
#TODO Cambiar res por platform

# Set Shader Binary Files Location.
SHADERS_BIN:=$(SHA_SOURCES:$(SHADIR)/%=$(SHADIR)/%.spv)

# C Compilers.
CC:=gcc
GLSLC:=glslc

# Compiling Flags.
CFLAGS:=-g -Wall -Wextra -Wconversion -D_DEBUG
# LIB_CFLAGS:=-g -Wall -Wextra -Wconversion -D_DEBUG -DOWL_EXPORTS
# -I$(VULKAN_SDK)/Include
# -I$(LIB_SRCDIR)
# -DNDEBUG (for releases)

# Linking Flags.
LFLAGS:=-s -L$(VULKAN_SDK)/Lib -lvulkan-1
# APP_LFLAGS:=-g -L$(LIBDIR) -l$(LIBNAME) -L$(VULKAN_SDK)/Lib -lvulkan-1 -lpng
# LIB_LFLAGS:=-g -Wl,--subsystem,windows
# LIB_LFLAGS:=-g -fPIC -shared -Wl,--subsystem,windows
# -s (for release builds)
# -lgdi32 (SwapBuffers)
# -luser32
# -llua
# -lm (math)
# -mwindows (disable console in APP)
# -lpng
# -L$(VULKAN_SDK)/Lib
# -lvulkan-1
# -lglfw3dll

###

# APP run.
all: $(BINDIR)/$(EXEFILE)
	@echo "	Corriendo: [$(BINDIR)/$(EXEFILE)]"
	@$(BINDIR)/$(EXEFILE)

# Test App.
app: $(BINDIR)/$(EXEFILE)
$(BINDIR)/$(EXEFILE): $(OBJECTS) $(RES_OBJ)
	@$(CC) $^ $(LFLAGS) -o $@
	@echo "	Enlazado completado: [$@]"
	@echo

# Objects.
$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c $(SHADERS_BIN)
	@$(CC) -c $< $(CFLAGS) -o $@
	@echo "	Compilado correctamente: [$<]"

# Resource Object.
$(RES_OBJ): $(RES_SRC)
	@windres -i $< -D_DEBUG -o $@
	@echo "	Compilado correctamente: [$<]"

# Shaders.
shaders: $(SHADERS_BIN)
$(SHADERS_BIN): $(SHADIR)/%.spv : $(SHADIR)/%
	@$(GLSLC) $< -o $@
	@echo "	Compilado correctamente: [$<]"

.PHONY: clean
# Clean All Objects.
clean:
	@$(RM) $(SHADERS_BIN)
	@echo "	Shaders limpiados."
	@$(RM) $(OBJECTS)
	@$(RM) $(RES_OBJ)
	@echo "	Objetos limpiados."

# Remove All Binary Files.
remove: clean
	@$(RM) $(BINDIR)/$(EXEFILE)
	@echo "	Binarios eliminados."

debug:
	@gdb $(BINDIR)/$(EXEFILE)