bin_PROGRAMS = ltris2
ltris2_SOURCES = main.cpp vconfig.h vconfig.cpp sdl.h sdl.cpp mixer.h \
	tools.h tools.cpp theme.h theme.cpp view.h view.cpp \
	menu.h menu.cpp
ltris2_LDADD = ../libgame/libgame.a
	
SUBDIRS = themes

install-data-local:
	$(mkinstalldirs) $(DESTDIR)/$(datadir)/ltris2

uninstall-local:
	rm -rf $(DESTDIR)$(datadir)/ltris2

clean:
	rm -f *.o ltris2
