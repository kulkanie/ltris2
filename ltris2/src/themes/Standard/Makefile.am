SUBDIRS = wallpapers

EXTRA_DIST = CREDITS SIL-OFL.txt theme.ini \
small.otf f_normal.otf menu.png \
s_menuclick.wav s_menumotion.wav 

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datdir)/themes/Standard
	@for file in $(EXTRA_DIST); do\
	  $(INSTALL_DATA) $$file $(DESTDIR)$(datdir)/themes/Standard/$$file;\
	done
