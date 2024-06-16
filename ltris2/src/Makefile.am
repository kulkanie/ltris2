bin_PROGRAMS = ltris2
ltris2_SOURCES = main.cpp vconfig.h vconfig.cpp sdl.h sdl.cpp mixer.h \
	tools.h tools.cpp theme.h theme.cpp view.h view.cpp \
	menu.h menu.cpp vcharts.h vcharts.cpp vbowl.h vbowl.cpp \
	vgame.h vgame.cpp
ltris2_LDADD = ../libgame/libgame.a
	
EXTRA_DIST = empty.hscr figures
	
SUBDIRS = themes

install-data-local:
	$(mkinstalldirs) $(DESTDIR)/$(datadir)/ltris2
	$(INSTALL_DATA) figures $(DESTDIR)/$(datadir)/ltris2/figures
	$(mkinstalldirs) $(DESTDIR)/$(hiscoredir)
	if ! test -f $(DESTDIR)/$(hiscoredir)/ltris2.hscr; then \
	  $(INSTALL_DATA) --mode=666 empty.hscr $(DESTDIR)/$(hiscoredir)/ltris2.hscr; \
	fi;


uninstall-local:
	rm -rf $(DESTDIR)/$(datadir)/ltris2
	rm -f  $(DESTDIR)/$(hiscoredir)/ltris2.hscr

clean:
	rm -f *.o ltris2
