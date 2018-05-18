shell: parser.c file.c shell.c internal.c
	gcc -o shell parser.c internal.c shell.c file.c
