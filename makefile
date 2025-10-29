# ===== Project config =====
APP       := game
CXX       := g++
CXXFLAGS  := -std=c++17 -O2 -Wall -Wextra

SRCDIR    := src
OBJDIR    := build/obj
BINDIR    := build

# ===== Raylib (vendored) =====
RAYLIB_DIR := external/raylib
RAYLIB_SRC := $(RAYLIB_DIR)/src
RAYLIB_LIB := $(RAYLIB_SRC)/libraylib.a

# Include dirs: raylib headers live in its src/ and src/external/
RL_INC := -I$(RAYLIB_SRC) -I$(RAYLIB_SRC)/external

# Link system libs needed by raylib (Linux/X11)
SYS_LIBS := -lGL -lm -lpthread -ldl -lrt -lX11

# rpath not needed when fully static, but keep in case you later switch to shared
LD_RPATH := -Wl,--enable-new-dtags

# ===== Sources (recursive) =====
SRCS := $(shell find $(SRCDIR) -type f \( -name '*.cpp' -o -name '*.cc' -o -name '*.cxx' \))
ifeq ($(strip $(SRCS)),)
$(error No C++ sources found under $(SRCDIR). Create src/main.cpp with int main(){...})
endif

OBJS := $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(SRCS))
OBJS := $(patsubst %.cpp,%.o,$(patsubst %.cc,%.o,$(patsubst %.cxx,%.o,$(OBJS))))

CXXFLAGS += $(RL_INC)
LDFLAGS  += $(LD_RPATH) $(RAYLIB_LIB) $(SYS_LIBS)

.PHONY: all run clean debug raylib print

all: $(BINDIR)/$(APP)

# Build raylib static lib first
$(RAYLIB_LIB):
	$(MAKE) -C $(RAYLIB_SRC) PLATFORM=PLATFORM_DESKTOP RAYLIB_RELEASE=1

# Link your app
$(BINDIR)/$(APP): $(RAYLIB_LIB) $(OBJS)
	@mkdir -p $(BINDIR)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# Compile pattern rules
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cxx
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	@echo "→ Running $(BINDIR)/$(APP)"
	@$(BINDIR)/$(APP)

clean:
	@echo "→ Cleaning build artifacts"
	@rm -rf $(BINDIR) $(OBJDIR)
	@$(MAKE) -C $(RAYLIB_SRC) clean >/dev/null || true

debug: CXXFLAGS += -g -O0
debug: clean all

print:
	@echo "Sources:"; printf '  %s\n' $(SRCS)
	@echo "Using RL_INC : $(RL_INC)"
	@echo "Using LDFLAGS: $(LDFLAGS)"
