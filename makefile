chatmake: chat.c my_msq.h my_sem.h
	gcc -Wall -pedantic chat.c my_msq.h my_sem.h -o chat
