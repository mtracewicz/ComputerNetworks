#include <sys/types.h> 
#include <sys/stat.h>  
#include <stdio.h>     
#include <unistd.h>    
#include <dirent.h>    
#include <string.h>    
#include <stdlib.h>   
#include <time.h>     
#include <pwd.h>       
#include <grp.h>       
#include <fcntl.h>
#include <getopt.h>

/*flagi do -h i -t odpowiednio*/
int f_1,f_2;

/*funkcja do liczenia wielkosci pliku w jednostkach czytelnych K,M,G,T*/
int readableval(long *number){
	int counter = 0;
	while(*number >= 1024 )
	{
		*number = *number / 1024;
		counter++;
	};
	return counter;
};

/*funkcja stprawdza czy plik to katalog czy plik regularny*/
static char ftyp (mode_t bit){
  if (S_ISREG (bit))
    return '-';
  if (S_ISDIR (bit))
    return 'd';

  return '0';
}

/*funkcja zapisujaca prawa dostepu do pliku w formacie drwxrwxrwx*/
void smod (mode_t mode, char *acces){
  acces[0] = ftyp (mode);/*wywolanie funkcji sprawdzajacej czy dany plik to katalog*/
  acces[1] = mode & S_IRUSR ? 'r' : '-';
  acces[2] = mode & S_IWUSR ? 'w' : '-';
  acces[3] = (mode & S_ISUID
            ? (mode & S_IXUSR ? 's' : 'S')
            : (mode & S_IXUSR ? 'x' : '-'));
  acces[4] = mode & S_IRGRP ? 'r' : '-';
  acces[5] = mode & S_IWGRP ? 'w' : '-';
  acces[6] = (mode & S_ISGID
            ? (mode & S_IXGRP ? 's' : 'S')
            : (mode & S_IXGRP ? 'x' : '-'));
  acces[7] = mode & S_IROTH ? 'r' : '-';
  acces[8] = mode & S_IWOTH ? 'w' : '-';
  acces[9] = (mode & S_ISVTX
            ? (mode & S_IXOTH ? 't' : 'T')
            : (mode & S_IXOTH ? 'x' : '-'));
  acces[10]=' ';
  acces[11]='\0';
}

/*glowna funkcja wypisujaca zawartosc katalogu, jako argumenty przyjmuje 
  nazwe katalogu do wyswietlenia i nazwe katalogu z ktorego wywolalismy program*/
void list_dir(char *name,char *cwd)
{
    /*zmienne zapisujace informacje o katalogu, deskryptorze pliku i jego wlasnosciach*/	
    DIR *dir;
    struct dirent *dp;
    struct stat statbuf;
    /*zmienne do zapisu czasu w formacie czytelnym dla uzytkownika*/
    char mtime[80];
    time_t t;
    struct tm nt;
    /*zmienne do zapisu praw dostepu*/
    mode_t mode;
    char acces[12];
    /*zmienna do zapisu wielkosci pliku*/
    long number;
  
    printf("%s\n",name);/*wypisanie nazwy katalogu*/
    chdir(name);/*zmieniamy katalog roboczy na podany w argumencie funkcji*/
    
    /*otwieramy obecny katalog roboczy i zapisujemy go do zmiennej dir,
     lub zwracamy blad przy niepowodzeniu*/
    if ((dir = opendir(".")) == NULL) 
    {
        perror("Blad");
    }
    
    /*tak dlugo jak mozemy otworzyc kolejny plik w katalogu wyswietlamy o nim informacje*/
    while ((dp = readdir(dir)) != NULL) 
    {
	/*zapisujemy dane pliku do statbuf, lub omijamy plik w momencie niepowodzenia*/    
        if (stat(dp->d_name, &statbuf) == -1)
            continue;
        
	/*konwertujemy prawa dostepu*/
    	mode=statbuf.st_mode;
    	smod(mode,acces);
	/*konwertujemy czas ostatniej modyfikacji*/
	t=statbuf.st_mtime;
	localtime_r(&t, &nt);
	strftime(mtime,sizeof(mtime),"%d %b %H:%M",&nt);
	/*przypisujemy zmiennej number wielkosc pliku*/
	number = statbuf.st_size;

	/* jezeli nie ma flagi -h */
	if(f_1 == 0)
	{
		printf("%s %4d %s  %s %7ld %s %s\n",
		       acces,/*prawa dostempu*/
		       statbuf.st_nlink,/*liczba dowiazan*/
		       getpwuid(statbuf.st_uid)->pw_name,/*nazwa wlasciciela*/
		       getgrgid(statbuf.st_gid)->gr_name,/*grupa wlasciciela*/
		       number,/*rozmiar*/
		       mtime,/*czas ostatniej modyfikacji*/
		       dp->d_name);/*nazwa pliku*/   
	}
	else
	{
		/*zmienne do konwersji z B na KB,MB,GB,TB*/
		int counter;
		char unit;
		
		/*wywolanie funkcji zmieniajacej numer i zwracajacej counter,
		 ktory posluzy do okreslenia jednostki*/
		counter = readableval(&number);	
		
		/*wybieramy jednostke na podstawie zmiennej counter*/
		switch(counter)
		{
			case 1:
				unit = 'K';
				break;
			case 2:
				unit = 'M';
				break;
			case 3:
				unit = 'G';
				break;
			case 4:
				unit = 'T';
				break;
			default:
				unit = ' ';
				break;
		}
		/*wypisujemy jak w poprzednim wypadku, zmienna unit odpowiada za jednostke*/
		printf("%s %4d %s  %s %7.1ld%c %s %s\n",
		       acces,
		       statbuf.st_nlink,
		       getpwuid(statbuf.st_uid)->pw_name,
		       getgrgid(statbuf.st_gid)->gr_name,
		       number,
		       unit,
		       mtime,
		       dp->d_name); 
	}
    }

    /*zamykamy folder na ktorym pracujemy i wracamy z katalogiem roboczym do katalogu wywolania programu*/
    closedir(dir);
    chdir(cwd);
}

int main(int argc,char** argv)
{
	int i,o,long_index = 0;
	
	/*zmienna zapisujaca katalog z ktorego uruchamiamy program*/
	char cwd[256];
	
	/*zarujemy flagi*/
	f_1 = 0;
	f_2 = 0;

	/*struktura w ktorej okreslamy opcje dlugi(tj. zaczynajace sie od -- */	
	static const struct option long_options[] =
	{
		/*w tym wypadku obie opcje nie wymagaja argumentow,
		 dla help zwracamy skrot s a dla version v*/
		{"help",no_argument,0,'s'},
		{"version",no_argument,0,'v'}
	};	

	/*pobieramy aktualny katlog, lub zwracamy blad w wypadku niepowodzenia*/
	if(getcwd(cwd, sizeof(cwd)) == NULL)
		perror("getcwd() error");
	
	/*jezeli nie podalismy zadnych argumentow wywolujemy program w aktualnym katalogu*/
	if(argc == 1)
	{
		list_dir(cwd,cwd);
	}
	else
	{
		/*sprawdzamy podane opcje*/
		while((o = getopt_long(argc,argv,"thsv",long_options,&long_index)) > -1)
		{
			switch(o)
			{
				/* -h zamiana B na KB,MB,GB,TB */
				case 'h':
					f_1 = 1;
					break;		
				/* -t sortowanie po czasie(niezaimplementowane)*/
				case 't':
					f_2 = 1;
					break;
				/* --help */
				case 's':
					printf("Mozna uzyc opcji -t(nie zaimplementowana, zadzaiala jak normalne wywolanie) -h i --version program bez argumentow dziala jak wywolanie ls -la\n Dodatkowe informacje mtracewicz@mat.umk.pl\n");	
					exit(3);
				/* --version */
				case 'v':
					printf("Wersja do oddania 31.10.2018r. Michal Tracewicz mtracewicz@mat.umk.pl\n");
					exit(2);
				/*gdy podamy argument zaczynajacy sie na - lub -- inny od dozwolonych*/	
				default:
					printf("Uses: %s [-t] [-h] [--help] [--version]\n",argv[0]);
					exit(1);
			}
		}
		/*uruchamiamy programy z podanymi argumentami*/
		for(i=1;i<argc;i++)
		{
			/*if pozwala na poprawne wywolanie programu przy podaniu wielu argumentow*/
			if(argc>2)	
			{
				/*omijamy wyswietlenie jezeli argument wynosi -h lub -t*/
				if((strcmp(*(argv+i),"-h") == 0) || (strcmp(*(argv+i),"-t") == 0))
					continue;
				/*wypisujemy katalog podany w argumencie*/
				list_dir(*(argv+i),cwd);
			}

			/*else zapewnia poprawne wywolanie dla ./my_ls -h i ./my_ls -t
			 oraz w wypadku wywolania ./my_ls z jednym katalogiem jako argumentem*/
			else	
			{	
				if((strcmp(*(argv+i),"-h") == 0) || (strcmp(*(argv+i),"-t") == 0))
					list_dir(cwd,cwd);
				else
					list_dir(*(argv+i),cwd);	
			}
		}
	}
	return 0;
}

