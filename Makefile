# ============================================================================
# PlexChem — AP Chemistry Ch. 12 Kinetics for TI-84 Plus CE (OS 5.3 - 5.8.4)
# Supports BOTH:
#   1) Flash App via CE-Programming/app_tools  (make app  -> APPS menu, 0 RAM!)
#   2) Standalone .8xp program                 (make      -> bin/PLEXCHEM.8xp)
#   3) Desktop 320x240 simulator & unit tests  (make sim)
#
# ZERO external .8xv libraries required (no graphx/fontlibc/keypadc needed)!
# ============================================================================

NAME        := PLEXCHEM
DESCRIPTION := "PlexChem AP Chem Ch.12 Kinetics"
COMPRESSED  := YES
ARCHIVED    := YES

# Flash App settings (when building with CE-Programming/app_tools)
APP_NAME    := PlexChem
APP_SHORT   := PLEXCHEM
INSTALLER   := CHMSETUP

CFLAGS      := -Wall -Wextra -Oz
CXXFLAGS    := -Wall -Wextra -Oz

# Default CE-Programming/toolchain include
CEDEV ?= $(shell cedev-config --prefix 2>/dev/null || echo /opt/CEdev)
ifneq ($(wildcard $(CEDEV)/meta/makefile.mk),)
include $(CEDEV)/meta/makefile.mk
endif

# Optional app_tools Flash App build target (requires CE-Programming/app_tools)
.PHONY: app sim fonts
app:
	@echo "Building PlexChem Flash App via CE-Programming/app_tools..."
	$(MAKE) -f $(CEDEV)/meta/makefile.mk APP_TOOLS=1 NAME=$(APP_SHORT)

fonts:
	python3 tools/bake_fonts.py

sim:
	gcc -O2 -Wall -Wextra -DHOST_SIM \
		src/hal_ce.c src/gfx.c src/math_render.c src/kinetics.c src/ui_app.c tools/test_and_render.c \
		-lm -o test_and_render
	./test_and_render
