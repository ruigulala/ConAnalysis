Apache-25520
Compile command:
> CC=wllvm CFLAGS='-g -O0 -fPIC' ./configure --with-mpm=worker --prefix=`pwd`/../apache-install --with-devrandom=/dev/urandom --disable-so --enable-cache=static --enable-mem_cache=static --disable-proxy --with-included-apr -enable-so=static --enable-maintainer-mode

> make

> make install
