# ---------------------------------------------------------------
# Makefile — tarsau projesi
#
# Kullanim:
#   make          → derle  (bin/tarsau oluşur)
#   make clean    → obj/ ve bin/ içini temizle
# ---------------------------------------------------------------

all:
	mkdir -p obj bin
	gcc -Wall -Wextra -pedantic -std=c99 -Iinclude -c src/main.c    -o obj/main.o
	gcc -Wall -Wextra -pedantic -std=c99 -Iinclude -c src/bundle.c  -o obj/bundle.o
	gcc -Wall -Wextra -pedantic -std=c99 -Iinclude -c src/extract.c -o obj/extract.o
	gcc -Wall -Wextra -pedantic -std=c99 -Iinclude -o bin/tarsau obj/main.o obj/bundle.o obj/extract.o

clean:
	rm -f obj/main.o obj/bundle.o obj/extract.o bin/tarsau

.PHONY: all clean
