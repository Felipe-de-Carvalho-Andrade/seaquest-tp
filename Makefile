ALLEGRO_VERSION=5.0.10
MINGW_VERSION=4.7.0
FOLDER=C:

FOLDER_NAME=\allegro-$(ALLEGRO_VERSION)-mingw-$(MINGW_VERSION)
PATH_ALLEGRO=$(FOLDER)$(FOLDER_NAME)
LIB_ALLEGRO=\lib\liballegro-$(ALLEGRO_VERSION)-monolith-mt.a
INCLUDE_ALLEGRO=\include

all: seaquest.exe

seaquest.exe: seaquest.o
	gcc -o seaquest.exe seaquest.o $(PATH_ALLEGRO)$(LIB_ALLEGRO)

seaquest.o: seaquest.c
	gcc -I $(PATH_ALLEGRO)$(INCLUDE_ALLEGRO) -c seaquest.c		
	
clean:
	del seaquest.o 
	del seaquest.exe
