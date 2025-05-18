# If you move this project you can change the directory
# to match your GBDK root directory (ex: GBDK_HOME = "C:/GBDK/"
ifndef GBDK_HOME
	GBDK_HOME = ../../../
endif

LCC = $(GBDK_HOME)bin/lcc
PNG2ASSET = $(GBDK_HOME)bin/png2asset

# Set platforms to build here, spaced separated. (These are in the separate Makefile.targets)
# They can also be built/cleaned individually: "make gg" and "make gg-clean"
# Possible are: gb gbc pocket megaduck sms gg
TARGETS= gb # megaduck # gb pocket megaduck sms gg nes

# Configure platform specific LCC flags here:
LCCFLAGS_gb      = -Wl-yt0x1B # Set an MBC for banking (1B-ROM+MBC5+RAM+BATT)
LCCFLAGS_pocket  = # -Wl-yt0x1B # Usually the same as required for .gb
LCCFLAGS_duck    = # -Wl-yt0x1B # Usually the same as required for .gb
LCCFLAGS_gbc     = # -Wl-yt0x1B -Wm-yc # Same as .gb with: -Wm-yc (gb & gbc) or Wm-yC (gbc exclusive)
LCCFLAGS_sms     =
LCCFLAGS_gg      =
LCCFLAGS_nes     =

LCCFLAGS += $(LCCFLAGS_$(EXT)) # This adds the current platform specific LCC Flags

LCCFLAGS += -Wl-j -Wm-ya4 -autobank -Wb-ext=.rel -Wb-v # MBC + Autobanking related flags
LCCFLAGS += -debug # Uncomment to enable debug output
# LCCFLAGS += -v     # Uncomment for lcc verbose output
# CFLAGS += -v       # Uncomment for compile stage verbose output
LCCFLAGS += -Wf-MMD -Wf-Wp-MP # Header file dependency output (-MMD) for Makefile use + per-header Phony rules (-MP)
CFLAGS += -Wf-MMD -Wf-Wp-MP # Header file dependency output (-MMD) for Makefile use + per-header Phony rules (-MP)


# Set CGB Boot ROM color palette to 0x13
# 1. Old Licensee is already 0x33 -> Use New Licensee
# 2. Sets New Licensee to "01" "(Nintendo)
# 3. (Calculated by Sum of ROM Header title bytes 0x134 - 0x143) & 0xFF = 0x58 = Grey CGB palette (id:0x16 -> checksum 0x58)  
#    https://gbdev.io/pandocs/Power_Up_Sequence.html#compatibility-palettes
#    https://tcrf.net/Notes:Game_Boy_Color_Bootstrap_ROM#Manual_Select_Palette_Configurations
# Set ROM Title / Name
LCCFLAGS += -Wm-yn"SHOWDOWNDEMO>>>"
LCCFLAGS += -Wm-yk01

# You can set the name of the ROM file here
PROJECTNAME = gbshowdown25

# EXT?=gb # Only sets extension to default (game boy .gb) if not populated
SRCDIR      = src
OBJDIR      = obj/$(EXT)
RESOBJSRC   = $(OBJDIR)/res
RESDIR      = res
BINDIR      = build/$(EXT)
MKDIRS      = $(OBJDIR) $(BINDIR) $(RESOBJSRC) # See bottom of Makefile for directory auto-creation

CFLAGS += -I$(RESOBJSRC)

# For png2asset: converting source pngs -> .c -> .o
IMGPNGS     = $(foreach dir,$(RESDIR),$(notdir $(wildcard $(dir)/*.png)))
IMGSOURCES  = $(IMGPNGS:%.png=$(RESOBJSRC)/%.c)
IMGOBJS     = $(IMGSOURCES:$(RESOBJSRC)/%.c=$(OBJDIR)/%.o)

BINS	    = $(OBJDIR)/$(PROJECTNAME).$(EXT)
CSOURCES    = $(foreach dir,$(SRCDIR),$(notdir $(wildcard $(dir)/*.c))) $(foreach dir,$(RESDIR),$(notdir $(wildcard $(dir)/*.c)))

ASMSOURCES  = $(foreach dir,$(SRCDIR),$(notdir $(wildcard $(dir)/*.s)))
OBJS        = $(CSOURCES:%.c=$(OBJDIR)/%.o) $(ASMSOURCES:%.s=$(OBJDIR)/%.o)

# Dependencies (using output from -Wf-MMD -Wf-Wp-MP)
DEPS = $(OBJS:%.o=%.d)

-include $(DEPS)

# Builds all targets sequentially
all: $(TARGETS)

# Use png2asset to convert the png into C formatted metasprite data
# Convert metasprite .pngs in res/ -> .c files in obj/<platform ext>/src/
$(RESOBJSRC)/%.c:	$(RESDIR)/%.png
	$(PNG2ASSET) $< `cat <$<.meta 2>/dev/null` -c $@

# Compile the pngs that were converted to .c files
# .c files in obj/res/ -> .o files in obj/
$(OBJDIR)/%.o:	$(RESOBJSRC)/%.c
	$(LCC) $(LCCFLAGS) $(CFLAGS) -c -o $@ $<


# Compile .c files in "src/" to .o object files
$(OBJDIR)/%.o:	$(SRCDIR)/%.c
	$(LCC) $(CFLAGS) -c -o $@ $<

# Compile .c files in "res/" to .o object files
$(OBJDIR)/%.o:	$(RESDIR)/%.c
	$(LCC) $(CFLAGS) -c -o $@ $<

# Compile .s assembly files in "src/" to .o object files
$(OBJDIR)/%.o:	$(SRCDIR)/%.s
	$(LCC) $(CFLAGS) -c -o $@ $<

# If needed, compile .c files in "src/" to .s assembly files
# (not required if .c is compiled directly to .o)
$(OBJDIR)/%.s:	$(SRCDIR)/%.c
	$(LCC) $(CFLAGS) -S -o $@ $<

# Convert images first so they're available when compiling the main sources
$(OBJS):	$(IMGOBJS)

# Link the compiled object files into a .gb ROM file
$(BINS):	$(OBJS)
	$(LCC) $(LCCFLAGS) -o $(BINDIR)/$(PROJECTNAME).$(EXT) $(OBJS) $(IMGOBJS)

clean:
	@echo Cleaning
	@for target in $(TARGETS); do \
		$(MAKE) $$target-clean; \
	done

# Include available build targets
include Makefile.targets


# create necessary directories after Makefile is parsed but before build
# info prevents the command from being pasted into the makefile
ifneq ($(strip $(EXT)),)           # Only make the directories if EXT has been set by a target
$(info $(shell mkdir -p $(MKDIRS)))
endif
