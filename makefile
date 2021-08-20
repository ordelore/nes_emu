# ----------------------------
# Makefile Options
# ----------------------------

NAME ?= AGNECE
ICON ?= icon.png
DESCRIPTION ?= "NES Emulator"
COMPRESSED ?= YES
ARCHIVED ?= NO

CFLAGS ?= -Wall -Wextra -O3

# ----------------------------

ifndef CEDEV
$(error CEDEV environment path variable is not set)
endif

include $(CEDEV)/meta/makefile.mk
