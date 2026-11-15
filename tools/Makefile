# Build the test tools.
#
#   make            everything that the installed libraries allow
#   make x11        xinject, xgrab, xlist
#   make wayland    wlxinject, screencopy
#   make clean
#
# X11 tools need libX11, libXtst (xinject) and libXcomposite (xgrab); the
# Wayland tools need libwayland-client and wayland-scanner, and generate the
# protocol glue from the XML in ../scripts/protocols, like the gates do.
#
# The binaries land in build/ so the source directory stays clean.

CC      ?= cc
CFLAGS  ?= -O2 -Wall
SCANNER ?= wayland-scanner

BUILD   := build
PROTOS  := ../scripts/protocols

WAYLAND_CLIENT_CFLAGS := $(shell pkg-config --cflags wayland-client 2>/dev/null)
WAYLAND_CLIENT_LIBS   := $(shell pkg-config --libs wayland-client 2>/dev/null)

TARGETS :=

ifneq ($(shell pkg-config --exists x11 xtst xcomposite && echo yes),)
TARGETS += x11
endif
ifneq ($(shell pkg-config --exists wayland-client && command -v wayland-scanner >/dev/null && echo yes),)
TARGETS += wayland
endif

all: $(TARGETS)

x11: $(BUILD)/xinject $(BUILD)/xgrab $(BUILD)/xlist

wayland: $(BUILD)/wlxinject $(BUILD)/screencopy $(BUILD)/kwininject

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/xinject: xinject.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $< $(shell pkg-config --cflags --libs x11 xtst)

$(BUILD)/xgrab: xgrab.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $< $(shell pkg-config --cflags --libs x11 xcomposite)

$(BUILD)/xlist: xlist.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $< $(shell pkg-config --cflags --libs x11)

$(BUILD)/wlr-virtual-pointer-protocol.c: $(PROTOS)/wlr-virtual-pointer-unstable-v1.xml | $(BUILD)
	$(SCANNER) private-code $< $@

$(BUILD)/wlr-virtual-pointer-unstable-v1-client-protocol.h: $(PROTOS)/wlr-virtual-pointer-unstable-v1.xml | $(BUILD)
	$(SCANNER) client-header $< $@

$(BUILD)/org-kde-kwin-fake-input-client-protocol.h: $(PROTOS)/org-kde-kwin-fake-input.xml | $(BUILD)
	$(SCANNER) client-header $< $@

$(BUILD)/org-kde-kwin-fake-input-protocol.c: $(PROTOS)/org-kde-kwin-fake-input.xml | $(BUILD)
	$(SCANNER) private-code $< $@

$(BUILD)/wlr-screencopy-protocol.c: $(PROTOS)/wlr-screencopy-unstable-v1.xml | $(BUILD)
	$(SCANNER) private-code $< $@

$(BUILD)/wlr-screencopy-client.h: $(PROTOS)/wlr-screencopy-unstable-v1.xml | $(BUILD)
	$(SCANNER) client-header $< $@

$(BUILD)/wlxinject: wlxinject.c $(BUILD)/wlr-virtual-pointer-protocol.c $(BUILD)/wlr-virtual-pointer-unstable-v1-client-protocol.h | $(BUILD)
	$(CC) $(CFLAGS) -I$(BUILD) $(WAYLAND_CLIENT_CFLAGS) -o $@ \
		wlxinject.c $(BUILD)/wlr-virtual-pointer-protocol.c $(WAYLAND_CLIENT_LIBS)

$(BUILD)/screencopy: screencopy.c $(BUILD)/wlr-screencopy-protocol.c $(BUILD)/wlr-screencopy-client.h | $(BUILD)
	$(CC) $(CFLAGS) -I$(BUILD) $(WAYLAND_CLIENT_CFLAGS) -o $@ \
		screencopy.c $(BUILD)/wlr-screencopy-protocol.c $(WAYLAND_CLIENT_LIBS)

$(BUILD)/kwininject: kwininject.c $(BUILD)/org-kde-kwin-fake-input-protocol.c $(BUILD)/org-kde-kwin-fake-input-client-protocol.h | $(BUILD)
	$(CC) $(CFLAGS) -I$(BUILD) $(WAYLAND_CLIENT_CFLAGS) -o $@ \
		kwininject.c $(BUILD)/org-kde-kwin-fake-input-protocol.c $(WAYLAND_CLIENT_LIBS)

clean:
	rm -rf $(BUILD)

.PHONY: all x11 wayland clean
