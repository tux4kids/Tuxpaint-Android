Summary: A drawing program for young children
Name: tuxpaint
Version: 0.9.22
Release: 1
Epoch: 1
License: GPL
Group: Multimedia/Graphics
URL: http://www.tuxpaint.org/
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
Requires: SDL >= 1.2.4 SDL_image SDL_mixer SDL_ttf SDL_Pango
Requires: libpng librsvg2 cairo libpaper fribidi
BuildRequires: SDL-devel >= 1.2.4 SDL_image-devel SDL_mixer-devel SDL_ttf-devel SDL_Pango-devel
BuildRequires: libpng-devel librsvg2-devel cairo-devel libpaper-devel fribidi-devel
BuildRequires: libgsf-devel libxml2-devel gtk2-devel gperf gettext

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
Requires: SDL-devel >= 1.2.4 SDL_image-devel SDL_mixer-devel SDL_ttf-devel SDL_Pango-devel
Requires: libpng-devel librsvg2-devel cairo-devel libpaper-devel fribidi-devel
Requires: libgsf-devel libxml2-devel gtk2-devel gperf gettext

%description devel
development files for tuxpaint plugins.

%prep
%setup -q

%build
make PREFIX=%{_prefix}

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{_sysconfdir}
mkdir -p $RPM_BUILD_ROOT/%{_bindir}
mkdir -p $RPM_BUILD_ROOT/%{_datadir}
mkdir -p $RPM_BUILD_ROOT/%{_mandir}

make PREFIX=%{_prefix} DESTDIR=$RPM_BUILD_ROOT install

find $RPM_BUILD_ROOT -name tuxpaint.desktop | sort | \
    sed -e "s@$RPM_BUILD_ROOT@@g" > filelist.icons
find $RPM_BUILD_ROOT -name tuxpaint.png | sort | \
    sed -e "s@$RPM_BUILD_ROOT@@g" >> filelist.icons
find $RPM_BUILD_ROOT -name tuxpaint.svg | sort | \
    sed -e "s@$RPM_BUILD_ROOT@@g" >> filelist.icons
find $RPM_BUILD_ROOT -name tuxpaint.xpm | sort | \
    sed -e "s@$RPM_BUILD_ROOT@@g" >> filelist.icons

rm -rf $RPM_BUILD_ROOT/usr/share/doc/tuxpaint*

%clean
rm -rf $RPM_BUILD_ROOT

%files -f filelist.icons
%defattr(-,root,root,-)
%config(noreplace) %{_sysconfdir}/tuxpaint/tuxpaint.conf
%doc docs/*
%{_datadir}/tuxpaint/*
%{_sysconfdir}/bash_completion.d/tuxpaint-completion.bash

%defattr(0755, root, root)
%{_bindir}/tuxpaint
%{_bindir}/tuxpaint-import

%{_prefix}/lib/tuxpaint/plugins/*.so

%defattr(0644, root, root)
%{_datadir}/locale/*/LC_MESSAGES/tuxpaint.mo
%{_datadir}/man/man1/*
%{_datadir}/man/*/man1/tuxpaint.1.*

%files devel
%doc magic/docs/*
%{_prefix}/include/tuxpaint/tp_magic_api.h
%{_prefix}/bin/tp-magic-config

%changelog
* Mon Aug 20 2012 <shin1@wmail.plala.or.jp> -
- Corrected 'Requires' and 'BuildRequires'

* Wed Dec 07 2011 <shin1@wmail.plala.or.jp> -
- Added bash-completion file

* Wed Jul 1 2009 <nbs@sonic.net> -
- Set version number 0.9.22

* Sun May 24 2009 <shin1@wmail.plala.or.jp> -
- For 0.9.21
- Added dependency for fribidi

* Tue Jun 17 2008 <shin1@wmail.plala.or.jp> -
- Actually set Epoch number

* Sat Apr 26 2008 <acahalan@gmail.com> -
- DESTDIR is the standard name, not PKG_ROOT

* Fri Mar 21 2008 <shin1@wmail.plala.or.jp> -
- Set version number 0.9.20
- Set Epoch number
- Requirements added for -devel package.

* Sun Mar 02 2008 <shin1@wmail.plala.or.jp> -
- 0.9.19
- Requires SDL_Pango
- Included magic tools
- Separated devel package

* Sat Jun 01 2007  <shin1@wmail.plala.or.jp> -
- Requires librsvg2 and libpaper

* Fri Sep 08 2006  <shin1@wmail.plala.or.jp> -
- New offical URL for tuxpaint (http://www.tuxpaint.org/).

* Mon Aug 07 2006  <shin1@wmail.plala.or.jp> -
- "DESTDIR" patch is no longer needed.

* Thu Nov 03 2005  Richard June <rjune[AT]lumensoftware.com - 0:0.9.14-0.lumen.0
- Ported from CVS for 0.9.15
- Replaced all instances of absolute paths with macro counterparts
- Reset buildroot to incorporate username of the builder
- Set Release value to 0.lumen.0 ( so as not to clobber any distros that provide it)
- Set a proper %changelog entry


* Thu Sep 15 2005  <shin1@wmail.plala.or.jp> -
- Do not force install desktop icons when Gnome and/or KDE are not installed.

* Sun Mar 27 2005  <shin1@wmail.plala.or.jp> -
- Some hicolor icons not installed were removed from file list

* Fri Jan 14 2005  <bill@newbreedsoftware.com> -
- Changed Group from Amusements/Games to Multimedia/Graphics

* Tue Sep 21 2004  <shin1@wmail.plala.or.jp> -
- Initial build for version 0.9.14
