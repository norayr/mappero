navigation_soundsdir = $(datadir)/sounds/mapper
navigation_sounds_DATA = \
	first-exit.spx \
	second-exit.spx \
	slight-left.spx \
	slight-right.spx \
	straight.spx \
	third-exit.spx \
	turn-left.spx \
	turn-right.spx

navigation_languages = \
	de_DE \
	en_US \
	fi_FI

install-data-local:
	for lang in $(navigation_languages); \
	do \
	    install -d $(DESTDIR)$(navigation_soundsdir)/$$lang; \
	    install -c -m 644 $$lang/*.spx $(DESTDIR)$(navigation_soundsdir)/$$lang; \
	done

