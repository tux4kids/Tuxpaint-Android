make distclean;
../launchConfigure.sh --with-arch=armv5te;

sed -i 's/^CFLAGS = .*/CFLAGS = /' fc-case/Makefile
sed -i 's/^CPPFLAGS = .*/CPPFLAGS = /' fc-case/Makefile
sed -i 's/^LDFLAGS = .*/LDFLAGS = /' fc-case/Makefile
sed -i 's/arm-linux-androideabi-//' fc-case/Makefile

sed -i 's/^CFLAGS = .*/CFLAGS = /' fc-lang/Makefile
sed -i 's/^CPPFLAGS = .*/CPPFLAGS = /' fc-lang/Makefile
sed -i 's/^LDFLAGS = .*/LDFLAGS = /' fc-lang/Makefile
sed -i 's/arm-linux-androideabi-//' fc-lang/Makefile

sed -i 's/^CFLAGS = .*/CFLAGS = /' fc-glyphname/Makefile
sed -i 's/^CPPFLAGS = .*/CPPFLAGS = /' fc-glyphname/Makefile
sed -i 's/^LDFLAGS = .*/LDFLAGS = /' fc-glyphname/Makefile
sed -i 's/arm-linux-androideabi-//' fc-glyphname/Makefile

sed -i 's/^CFLAGS = .*/CFLAGS = /' fc-arch/Makefile
sed -i 's/^CPPFLAGS = .*/CPPFLAGS = /' fc-arch/Makefile
sed -i 's/^LDFLAGS = .*/LDFLAGS = /' fc-arch/Makefile
sed -i 's/arm-linux-androideabi-//' fc-arch/Makefile

sed -i 's/^CFLAGS = .*/CFLAGS = /' doc/Makefile
sed -i 's/^CPPFLAGS = .*/CPPFLAGS = /' doc/Makefile
sed -i 's/^LDFLAGS = .*/LDFLAGS = /' doc/Makefile
sed -i 's/arm-linux-androideabi-//' doc/Makefile

make -j5 && make install;
