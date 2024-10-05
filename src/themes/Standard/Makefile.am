EXTRA_DIST = CREDITS SIL-OFL.txt theme.ini \
	f_normal.otf f_bold.otf \
	menu.png cursor.png blocks.png \
	s_menuclick.wav s_menumotion.wav \
	s_explosion.wav s_insert.wav s_nextlevel.wav s_shift.wav s_tetris.wav \
	wallpaper0.jpg wallpaper1.jpg wallpaper2.jpg wallpaper3.jpg \
	wallpaper4.jpg wallpaper5.jpg wallpaper6.jpg wallpaper7.jpg \
	wallpaper8.jpg wallpaper9.jpg

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datdir)/themes/Standard
	@for file in $(EXTRA_DIST); do\
	  $(INSTALL_DATA) $$file $(DESTDIR)$(datdir)/themes/Standard/$$file;\
	done
