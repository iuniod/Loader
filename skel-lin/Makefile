CC = gcc
CFLAGS = -fPIC -m32 -Wall
LDFLAGS = -m32
TEST?=default_value_if_not_set_in_environment
.PHONY: build
build: libso_loader.so

libso_loader.so: loader.o exec_parser.o
	$(CC) $(LDFLAGS) -shared -o $@ $^
	cp libso_loader.so ../checker-lin

exec_parser.o: loader/exec_parser.c loader/exec_parser.h
	$(CC) $(CFLAGS) -o $@ -c $<

loader.o: loader/loader.c
	$(CC) $(CFLAGS) -o $@ -c $<

pack:
	zip -r 322CA_Caramida_Iustina_Andreea_Tema1.zip loader/loader.c loader/exec_parser.c loader/exec_parser.h exec/exec.c test_prog/hello.S Makefile Makefile.example README.md

.PHONY: clean
clean:
	-rm -f exec_parser.o loader.o libso_loader.so
