Summary: A drawing program for young children
Name: tuxpaint
Version: 0.9.31
Release: 1%{?dist}
Epoch: 1
License: GPL
Group: Multimedia/Graphics
URL: https://tuxpaint.org/
Source0: https://downloads.sourceforge.net/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
Requires: SDL2 >= 2.0 SDL2_image SDL2_mixer SDL2_ttf SDL2_Pango SDL2_gfx libpaper fribidi xdg-utils libimagequant
BuildRequires: SDL2-devel >= 2.0 SDL2_image-devel SDL2_mixer-devel SDL2_ttf-devel SDL2_Pango-devel SDL2_gfx-devel
BuildRequires: librsvg2-devel libpaper-devel fribidi-devel gperf gettext >= 0.19.7 ImageMagick xdg-utils libimagequant-devel

%description
"Tux Paint" is a drawing program for young children.
It provides a simple interface and fixed canvas size,
and provides access to previous images using a thumbnail
browser (e.g., no access to the underlying file-system).

Unlike popular drawing programs like "The GIMP," it has a
very limited tool-set. However, it provides a much simpler
interface, and has entertaining, child-oriented additions
such as sound effects.

%package devel
Summary: development files for tuxpaint plugins.
Group: Development/Libraries
Requires: tuxpaint = %{version}
Requires: SDL2-devel >= 2.0 SDL2_image-devel SDL2_mixer-devel SDL2_ttf-devel SDL2_Pango-devel SDL2_gfx-devel
Requires: librsvg2-devel libpaper-devel fribidi-devel gperf

%description devel
development files for tuxpaint plugins.

%prep
%setup -q

%build
make PREFIX=%{_prefix} DOC_PREFIX=%{_docdir}/tuxpaint linux_ARCH_CFLAGS='-I/usr/include/imagequant -I/usr/include/freetype2'

%install
rm -rf $RPM_BUILD_ROOT
make PACKAGE_ONLY=yes \
     PREFIX=%{_prefix} DESTDIR=$RPM_BUILD_ROOT \
     DOC_PREFIX=$RPM_BUILD_ROOT%{_docdir}/tuxpaint \
     DEVDOC_PREFIX=$RPM_BUILD_ROOT%{_docdir}/tuxpaint/devel \
     install

# Scripts in this directory force dependency on python2 and fontforge
rm -rf $RPM_BUILD_ROOT%{_datadir}/tuxpaint/fonts/locale/zh_tw_docs

%post
update-desktop-database

%postun
update-desktop-database

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(755,root,root,755)
%{_bindir}/tuxpaint
%{_bindir}/tuxpaint-import
%{_prefix}/lib/tuxpaint/*

%defattr(644,root,root,755)
%config(noreplace) %{_sysconfdir}/tuxpaint/tuxpaint.conf
%{_sysconfdir}/bash_completion.d/tuxpaint-completion.bash
%{_docdir}/tuxpaint/*
%{_datadir}/tuxpaint/*
%{_datadir}/applications/tuxpaint*.desktop
%{_datadir}/icons/hicolor/*/apps/tuxpaint.png
%{_datadir}/metainfo/org.tuxpaint.Tuxpaint.appdata.xml
%{_datadir}/locale/*/LC_MESSAGES/tuxpaint.mo
%{_mandir}/man1/tuxpaint*.*
%{_mandir}/*/man1/tuxpaint*.*
%exclude %{_docdir}/tuxpaint/*/MAGIC-API.txt
%exclude %{_docdir}/tuxpaint/*/tp_magic_example.c
%exclude %{_docdir}/tuxpaint/*/html/MAGIC-API.html
%exclude %{_docdir}/tuxpaint/*/html/tp_magic_example.c

%files devel
%attr(755,root,root) %{_bindir}/tp-magic-config
%defattr(644,root,root,755)
%{_includedir}/tuxpaint/tp_magic_api.h
%{_mandir}/man1/tp-magic-config.*
%{_docdir}/tuxpaint/*/MAGIC-API.txt
%{_docdir}/tuxpaint/*/tp_magic_example.c
%{_docdir}/tuxpaint/*/html/MAGIC-API.html
%{_docdir}/tuxpaint/*/html/tp_magic_example.c

%changelog
* Sat Jul 08 2023 <dolphin6k@wmail.plala.or.jp> -
- Use PACKAGE_ONLY=yes for desktop icon installation

* Wed Jun 07 2023 <dolphin6k@wmail.plala.or.jp> -
- Added fullscreen launcher icon.

* Sat May 20 2023 <nbs@sonic.net> -
- Set version number 0.9.31

* Tue Apr 04 2023 <nbs@sonic.net> -
- Set version number 0.9.30

* Wed Mar 22 2023 <dolphin6k@wmail.plala.or.jp>
- Removed suffix "-sdl2" from the release tar ball.

* Fri Mar 10 2023 <dolphin6k@wmail.plala.or.jp>
- Magic docs to go the main package
- Magic devel docs to go the devel package
- Excluded outdated docs.

* Sun Dec 11 2022 <nbs@sonic.net> -
- Updated URL to HTTPS

* Wed Jun 29 2022 <dolphin6k@wmail.plala.or.jp> -
- Changed library requirements from SDL to SDL2
- Adapted to the change of naming rule of tar ball.

* Wed Jun 29 2022 <dolphin6k@wmail.plala.or.jp> -
- Set minimum version requirement for gettext

* Tue Jun 14 2022 <nbs@sonic.net> -
- Set version number 0.9.29

* Wed Dec 01 2021 <nbs@sonic.net> -
- Set version number 0.9.28

* Thu Oct 07 2021 <dolphin6k@wmail.plala.or.jp> -
- Set version number 0.9.27
- doc/Makefile no longer installed

* Sun Dec 27 2020 <nbs@sonic.net> -
- Set version number 0.9.26

* Mon Jun 22 2020 <nbs@sonic.net> -
- Set version number 0.9.25

* Fri May 1 2020 <dolphin6k@wmail.plala.or.jp> -
- Enabled using xdg-utils for installing icons.
- Wrong date in %changelog
- Re-organized %files section
- Correct path for 'tp-magic-config --plugindocprefix'

* Sat Mar 14 2020 <dolphin6k@wmail.plala.or.jp> -
- Disable target "install-xdg". Add ImageMagick for BuildReq.

* Thu Sep 26 2019 <nbs@sonic.net> -
- Set version number 0.9.24

* Sun Aug 19 2018 <nbs@sonic.net> -
- Set version number 0.9.23

* Mon Aug 20 2012 <dolphin6k@wmail.plala.or.jp> -
- Corrected 'Requires' and 'BuildRequires'

* Wed Dec 07 2011 <dolphin6k@wmail.plala.or.jp> -
- Added bash-completion file

* Wed Jul 1 2009 <nbs@sonic.net> -
- Set version number 0.9.22

* Sun May 24 2009 <dolphin6k@wmail.plala.or.jp> -
- For 0.9.21
- Added dependency for fribidi

* Tue Jun 17 2008 <dolphin6k@wmail.plala.or.jp> -
- Actually set Epoch number

* Sat Apr 26 2008 <acahalan@gmail.com> -
- DESTDIR is the standard name, not PKG_ROOT

* Fri Mar 21 2008 <dolphin6k@wmail.plala.or.jp> -
- Set version number 0.9.20
- Set Epoch number
- Requirements added for -devel package.

* Sun Mar 02 2008 <dolphin6k@wmail.plala.or.jp> -
- 0.9.19
- Requires SDL_Pango
- Included magic tools
- Separated devel package

* Fri Jun 01 2007  <dolphin6k@wmail.plala.or.jp> -
- Requires librsvg2 and libpaper

* Fri Sep 08 2006  <dolphin6k@wmail.plala.or.jp> -
- New offical URL for tuxpaint (http://www.tuxpaint.org/).

* Mon Aug 07 2006  <dolphin6k@wmail.plala.or.jp> -
- "DESTDIR" patch is no longer needed.

* Thu Nov 03 2005  Richard June <rjune[AT]lumensoftware.com - 0:0.9.14-0.lumen.0
- Ported from CVS for 0.9.15
- Replaced all instances of absolute paths with macro counterparts
- Reset buildroot to incorporate username of the builder
- Set Release value to 0.lumen.0 ( so as not to clobber any distros that provide it)
- Set a proper %changelog entry


* Thu Sep 15 2005  <dolphin6k@wmail.plala.or.jp> -
- Do not force install desktop icons when Gnome and/or KDE are not installed.

* Sun Mar 27 2005  <dolphin6k@wmail.plala.or.jp> -
- Some hicolor icons not installed were removed from file list

* Fri Jan 14 2005  <bill@newbreedsoftware.com> -
- Changed Group from Amusements/Games to Multimedia/Graphics

* Tue Sep 21 2004  <dolphin6k@wmail.plala.or.jp> -
- Initial build for version 0.9.14
