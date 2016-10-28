// =============== REJESTRACJA ===============

#include <stdio.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <ctype.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <string.h>

int ile_pacjentow;
int dzis; 
int pacjenciHousa; //ile pacjentow ma dany lekarz
int pacjenciWatsona;

int House[365]; //dni pracy lekarzy 0-nie pracuje, 1-pracuje
int Watson[365]; 

//typ komunikatu z pacjentem = 1, rejestracja
struct msgPacjent{
  long type;
  char imie[10];
  char nazwisko[20];
  char PESEL[15];
  char haslo[20];
  int id;
} pacjent;

//type=4
struct msgLekarz{
  long type;
  int nr_lekarza;
  int dzialanie; //1-przegląd pacjentów, 2-urlop
  int poczatek; //pierwszy dzien urlopu
  int koniec; // ostatni dzien urlopu
  int id; //id kolejki
} lekarz;

//typ komunikatu logowania = 2;
struct msgLogowanie{
  long type;
  char PESEL[15];
  char haslo[20];
  int id;
} logowanie;

//type=3
struct msgOdpowiedz{
  long type;
  char tresc[1500];
} komunikat;

//type=5
struct msgDzialaniePacjenta{
  long type;
  int dzialanie; //1-6
  int dzien; //do wyswietlania dostepnych lekarzy w danym dniu
  int lekarz; // do wyswietlania dostepnych terminow danego lekarza
  int id; //id kolejki pacjenta 
  char PESEL[15];
} dzialaniePacjenta;

struct Pacjent{
  char imie[10];
  char nazwisko[20];
  char PESEL[15];
  char haslo[20];
  
  int dzien;
  int lekarz; //1-House, 2-Watson
  int proba; //ile razy probowano sie zalogowac nieskutecznie
  int ponad2; // 1 – rejestracji dokonano z ponad dwumiesiecznym wyprzedzeniem, w przeciwnym razie – 0
  
} baza_pacjentow[100];

//wypelnia tablice pracy lekarzy
void init()
{
  int i;
  for(i=0; i<365; i++)
  {
    House[i]=1;
    Watson[i]=1;
  }
}

void przypominajka() //"wysyla" przypomnienie do pacjenta 2 tyg przed wizyta
{
    int i;
      for(i=0; i<ile_pacjentow; i++)
      {
	if(baza_pacjentow[i].ponad2==1)
	{
	  int x;
	  x=baza_pacjentow[i].dzien - dzis;
	  if(x<=14)
	  {
	    printf("Wyslano przypomnienie do %s %s\n", baza_pacjentow[i].imie, baza_pacjentow[i].nazwisko);
	    baza_pacjentow[i].ponad2=0;
	  }
	}
      } 
}


void obsluga_lekarza() 
{
  strcpy(komunikat.tresc, "");
  printf("oblsluga lekarza\n");
  int i;
  if(lekarz.dzialanie==1)
  {
    //przegląd pacjentów 
   
      for(i=0; i<ile_pacjentow; i++)
      {
	if(baza_pacjentow[i].lekarz==lekarz.nr_lekarza)
	{
	  strcat(komunikat.tresc, baza_pacjentow[i].imie);
	  strcat(komunikat.tresc, " ");
	  strcat(komunikat.tresc, baza_pacjentow[i].nazwisko);
	  strcat(komunikat.tresc, "\n");
	  
	}
      }
      printf("%s\n", komunikat.tresc);
      msgsnd(lekarz.id, &komunikat, sizeof(komunikat)-sizeof(long), 0);
  }
  else
  {
    //urlop 
    
    for(i=lekarz.poczatek; i<=lekarz.koniec; i++)
    {
      if(lekarz.nr_lekarza==1)
	House[i]=0;
      else Watson[i]=0;
    }
    
  }
}

void wypisz()
{
  int i;
      for(i=0; i<ile_pacjentow; i++)
      {
	printf("%s\n", baza_pacjentow[i].imie);
	printf("%s\n", baza_pacjentow[i].nazwisko);
	printf("Czy ponad 2?: %d\n", baza_pacjentow[i].ponad2);
	printf("Dzien: %d\n", baza_pacjentow[i].dzien);
      }
}
//sprawdza czy pesel pacjenta z komunikatu znajduje się w bazie pacjentow
bool sprawdz_PESEL()
{
  int i;
  bool jest=0;
  for(i=0; i<ile_pacjentow; i++)
  {    
    if( strcmp(baza_pacjentow[i].PESEL, pacjent.PESEL)==0 )
    {      
      jest=1;  
    }
  }
  return jest;
}

//obsluga rejestracji
void rejestracja_pacjenta()
{
  strcpy(komunikat.tresc, "");
  int tmp=dzis;
    if(sprawdz_PESEL() == 1)
    {
      //Jest już zarejestrowany
      printf("Juz jest\n");
      strcat(komunikat.tresc, "Użytkownik o takim PESEL juz jest w bazie!");
      msgsnd(pacjent.id, &komunikat, sizeof(komunikat)-sizeof(long), 0);
      //strcpy(komunikat.tresc, "");
    }
    else
    {
      //Dodajemy do bazy pacjentow
      printf("Dodajemy\n");
      strcpy(baza_pacjentow[ile_pacjentow].imie, pacjent.imie);
      strcpy(baza_pacjentow[ile_pacjentow].nazwisko, pacjent.nazwisko);
      strcpy(baza_pacjentow[ile_pacjentow].PESEL, pacjent.PESEL);
      strcpy(baza_pacjentow[ile_pacjentow].haslo, pacjent.haslo);
      
      //równoważne obciążenia lekarzy, jesli House ma wiecej/tyle samo pacjentow co Watson to przyjmuje Watson
      if(pacjenciHousa>=pacjenciWatsona)
      {
	printf("Watson");
	baza_pacjentow[ile_pacjentow].lekarz=2; //Watson
	pacjenciWatsona++;
	
	while(tmp<365)
	{
	  if( Watson[tmp+1]==1 )
	  {
	    baza_pacjentow[ile_pacjentow].dzien=tmp+1;
	    Watson[tmp+1]=0;
	    sprintf(komunikat.tresc, "Zarejestrowano na dzień: %d ", tmp+1);
	    strcat(komunikat.tresc, " do doktora Watsona\n");
	    msgsnd(pacjent.id, &komunikat, sizeof(komunikat)-sizeof(long), 0);
	    
	    break;
	  }
	  tmp++;
	}
	
      }
      else 
      {
	printf("House");
	baza_pacjentow[ile_pacjentow].lekarz=1; //House
	pacjenciHousa++;
	
	while(tmp<365)
	{
	  if( House[tmp+1]==1 )
	  {
	    baza_pacjentow[ile_pacjentow].dzien=tmp+1;
	    House[tmp+1]=0;
	    strcat(komunikat.tresc, "Zarejestrowano na dzień: ");
	    sprintf(komunikat.tresc, "Zarejestrowano na dzień: %d", tmp+1);
	    strcat(komunikat.tresc, " do doktora Housa\n");
	    msgsnd(pacjent.id, &komunikat, sizeof(komunikat)-sizeof(long), 0);
	    
	    break;
	  }
	  tmp++;
	}
      }
      
      
      int x;
      x=baza_pacjentow[ile_pacjentow].dzien - dzis;
      //printf("Dzis: %d\n", dzis);
     // printf("X: %d\n", x);
      if( x>60 ) 
	baza_pacjentow[ile_pacjentow].ponad2=1;
      else
	baza_pacjentow[ile_pacjentow].ponad2=0;
      
      ile_pacjentow++;
      printf("%s\n", komunikat.tresc);
     // wypisz();
    }  
}
void logowanie_pacjenta()
{
  strcpy(komunikat.tresc, "");
  int i;
  bool zalogowano;
  bool zablokowane=0;
  for(i=0; i<ile_pacjentow; i++)
  {
    if( (strcmp(baza_pacjentow[i].PESEL, logowanie.PESEL)==0) )
    {    
      if(strcmp(baza_pacjentow[i].haslo, logowanie.haslo)==0)
      {
      printf("\nZalogowano\n");
      zalogowano=1;
      baza_pacjentow[i].proba=0;
      
      strcpy(komunikat.tresc, "Zalogowano");
      msgsnd(logowanie.id, &komunikat, sizeof(komunikat)-sizeof(long), 0);
      }
      else
      {
	baza_pacjentow[i].proba++;
	if(baza_pacjentow[i].proba==4) zablokowane=1;
      }
    }
  }
  if(zablokowane==1)
  {
    printf("\nKonto zablokowane\n");
    strcpy(komunikat.tresc, "Konto zablokowane");
    msgsnd(logowanie.id, &komunikat, sizeof(komunikat)-sizeof(long), 0);
  }
  else if(zalogowano==0) 
  {
    printf("\nNiepoprawne hasło lub PESEL\n");
    strcpy(komunikat.tresc, "Niepoprawne hasło lub pesel");
    msgsnd(logowanie.id, &komunikat, sizeof(komunikat)-sizeof(long), 0);
  }
}

void dzialanie_pacjenta()
{
  strcpy(komunikat.tresc, "");
  char tmp[50];
  int i=0;
  //printf("nr dzialania %d\n:", dzialaniePacjenta.dzialanie);
  switch(dzialaniePacjenta.dzialanie)
  {
    case 1: //Wyświetl liste lekarzy w danym dniu
      if(!(House[dzialaniePacjenta.dzien]==0)) 
	strcat(komunikat.tresc, "dr House\n");
      if(!(Watson[dzialaniePacjenta.dzien]==0)) 
	strcat(komunikat.tresc, "dr Watson\n");
      printf("%s\n", komunikat.tresc);
      msgsnd(dzialaniePacjenta.id, &komunikat, sizeof(komunikat)-sizeof(long), 0);
      
      break;
      
    case 2: //Wyświetl dostepne terminy danego lekarza
      
      if(dzialaniePacjenta.lekarz==1) //dr House
      {
	for(i=dzis; i<365; i++)
	{
	    if (House[i]==1)
	    {
	      sprintf(tmp, "%d", i);
	      strcat(komunikat.tresc, tmp);
	      strcat(komunikat.tresc, "\n");
	    }
	}
      }
      else //dr Watson
      {
	  for(i=dzis; i<365; i++)
	{
	    if (Watson[i]==1)
	    {
	      sprintf(tmp, "%d", i);
	      strcat(komunikat.tresc, tmp);
	      strcat(komunikat.tresc, "\n");
	    }
	}
      }
      printf("%s\n", komunikat.tresc);
      msgsnd(dzialaniePacjenta.id, &komunikat, sizeof(komunikat)-sizeof(long), 0);
      break;
      
    case 3: //Wyświetl status wizyty
      for(i=0; i<ile_pacjentow; i++)
      {    
	if( strcmp(baza_pacjentow[i].PESEL, dzialaniePacjenta.PESEL)==0 )
	{      
	   strcat(komunikat.tresc, baza_pacjentow[i].imie);
	   strcat(komunikat.tresc, " ");
	   strcat(komunikat.tresc, baza_pacjentow[i].nazwisko);
	   strcat(komunikat.tresc, "\nDzień wizyty: ");
	   sprintf(tmp, "%d", baza_pacjentow[i].dzien);
	   strcat(komunikat.tresc, tmp);
	   strcat(komunikat.tresc, "\nLekarz: ");
	   if(baza_pacjentow[i].lekarz!=0)
	   {
	   if(baza_pacjentow[i].lekarz==1)
	     strcat(komunikat.tresc, "dr House\n");
	   else
	     strcat(komunikat.tresc, "dr Watson\n");
	   }
	   else
	   {
	     strcpy(komunikat.tresc, "Brak zarejestrowanej wizyty\n");
	   }
	   break;
	}
      }
      printf("%s\n", komunikat.tresc);
      msgsnd(dzialaniePacjenta.id, &komunikat, sizeof(komunikat)-sizeof(long), 0);
      break;
      
    case 4: //Zmiana terminu wizyty
      for(i=0; i<ile_pacjentow; i++)
      {    
	if( strcmp(baza_pacjentow[i].PESEL, dzialaniePacjenta.PESEL)==0 )
	{      
	  int data=baza_pacjentow[i].dzien;
	  
	   if(baza_pacjentow[i].lekarz==1 )
	   {
	     House[data]=1;
	     pacjenciHousa--;
	   }
	   else
	   {
	     Watson[data]=1;
	     pacjenciWatsona--;
	   } 
	   break;
	}
      }
      //przydzielenie nowego terminu:
      printf("I: %d", i);
      int tmp2=dzis;
      //równoważne obciążenia lekarzy
        if(pacjenciHousa>=pacjenciWatsona)
      {
	printf("Watson");
	baza_pacjentow[i].lekarz=2; //Watson
	pacjenciWatsona++;
	
	while(tmp2<365)
	{
	  if( Watson[tmp2+1]==1 )
	  {
	    baza_pacjentow[i].dzien=tmp2+1;
	    Watson[tmp2+1]=0;
	    sprintf(komunikat.tresc, "Zarejestrowano na dzień: %d ", tmp2+1);
	    strcat(komunikat.tresc, " do doktora Watsona\n");
	    msgsnd(dzialaniePacjenta.id, &komunikat, sizeof(komunikat)-sizeof(long), 0);
	    
	    break;
	  }
	  tmp2++;
	}
	
      }
      else 
      {
	printf("House");
	baza_pacjentow[i].lekarz=1; //House
	pacjenciHousa++;
	
	while(tmp2<365)
	{
	  if( House[tmp2+1]==1 )
	  {
	    baza_pacjentow[i].dzien=tmp2+1;
	    House[tmp2+1]=0;
	    strcat(komunikat.tresc, "Zarejestrowano na dzień: ");
	    sprintf(komunikat.tresc, "Zarejestrowano na dzień: %d", tmp2+1);
	    strcat(komunikat.tresc, " do doktora Housa\n");
	    msgsnd(dzialaniePacjenta.id, &komunikat, sizeof(komunikat)-sizeof(long), 0);
	    
	    break;
	  }
	  tmp2++;
	}
      }
      break;
      
    case 5: //Odwołanie wizyty
      for(i=0; i<ile_pacjentow; i++)
      {    
	if( strcmp(baza_pacjentow[i].PESEL, dzialaniePacjenta.PESEL)==0 )
	{      
	  int data=baza_pacjentow[i].dzien;
	  
	   if(baza_pacjentow[i].lekarz==1 )
	   {
	     House[data]=1;
	     pacjenciHousa--;
	   }
	   else
	   {
	     Watson[data]=1;
	     pacjenciWatsona--;
	   } 
	   
	   baza_pacjentow[i].lekarz=0;
	   baza_pacjentow[i].dzien=0;
	   break;
	}
      }
      strcpy(komunikat.tresc, "Odwołano wizytę\n");
      printf("%s\n", komunikat.tresc);
      msgsnd(dzialaniePacjenta.id, &komunikat, sizeof(komunikat)-sizeof(long), 0);
      break;
  }
}

int main(int argc, char* argv[])
{
  ile_pacjentow=0;
  dzis=0;
  komunikat.type=3;
  pacjenciHousa=0;
  pacjenciWatsona=0;
  
  int mid; //id kolejki komunikatów serwera
  mid = msgget(0x123, 0777 | IPC_CREAT);
  
  init();
  
  while(dzis<365) //odbieranie komunikatów
  {
    if(msgrcv(mid, &logowanie, sizeof(logowanie)-sizeof(long), 2, IPC_NOWAIT)!=-1) //logowanie
   {
     printf("logowanie\n");
     logowanie_pacjenta();
   }
   
   if(msgrcv(mid, &pacjent, sizeof(pacjent)-sizeof(long), 1, IPC_NOWAIT)!=-1) //rejestracja
   {
     printf("rejestracja\n");
    rejestracja_pacjenta();
   }
   
   if(msgrcv(mid, &lekarz, sizeof(lekarz)-sizeof(long), 4, IPC_NOWAIT)!=-1) //lekarz
   {
     printf("lekarz\n");
     obsluga_lekarza();
   }
   
   if(msgrcv(mid, &dzialaniePacjenta, sizeof(dzialaniePacjenta)-sizeof(long), 5, IPC_NOWAIT)!=-1) //dzialanie pacjenta
   {
     printf("dzialanie pacjenta\n");
     dzialanie_pacjenta();
   }
      // wypisz();
    przypominajka();
    sleep(2);
    dzis++; //kolejny dzien
  }
  
  
  return 0;
}
