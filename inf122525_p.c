// =============== PACJENT ===============

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

//typ komunikatu z pacjentem = 1, rejestracja
struct msgPacjent{
  long type;
  char imie[10];
  char nazwisko[20];
  char PESEL[15];
  char haslo[20];
  int id;
} pacjent;

//typ komunikatu logowania = 2
struct msgLogowanie{
  long type;
  char PESEL[15];
  char haslo[20];
  int id; //id kolejki
} logowanie;
//typ = 3
struct msgOdpowiedz{
  long type;
  char tresc[1500];
} komunikat;

struct msgDzialaniePacjenta{
  long type;
  int dzialanie; //1-6
  int dzien; //do wyswietlania dostepnych lekarzy w danym dniu
  int lekarz; // do wyswietlania dostepnych terminow danego lekarza
  int id; //id kolejki pacjenta  
  char PESEL[15];
} dzialaniePacjenta;

void menu_po_zalogowaniu(int mid, int id_ipc_pacjent)
{
  int x=0;
  int shutdown=0;
  
  while(!shutdown)
  {
    scanf("%d", &x);
    dzialaniePacjenta.dzialanie=x;
    
  switch(x)
  {
    case 1: //Wyświetl listę lekarzy w danym dniu
      printf("Podaj dzien, aby wyswietlnic dostepnych lekarzy:\n");
      scanf("%d", &dzialaniePacjenta.dzien);
      msgsnd(mid, &dzialaniePacjenta, sizeof(dzialaniePacjenta)-sizeof(long), 0); //wysylanie do serwera
      
      msgrcv(id_ipc_pacjent, &komunikat, sizeof(komunikat)-sizeof(long), 3, 0); //komunikat zwrotny
      printf("\n%s\n", komunikat.tresc);
      
      break;
      
    case 2: //Wyswietl dostępne terminy danego lekarza
      printf("1 - dr House\n2 - dr Watson\n Podaj nr lekarza: \n");
      scanf("%d", &dzialaniePacjenta.lekarz);
      msgsnd(mid, &dzialaniePacjenta, sizeof(dzialaniePacjenta)-sizeof(long), 0); //wysylanie do serwera
      
      msgrcv(id_ipc_pacjent, &komunikat, sizeof(komunikat)-sizeof(long), 3, 0); //komunikat zwrotny
      printf("\n%s\n", komunikat.tresc);
      break;
      
    case 3://Wyświetl status wizyty
    case 4: //Zmiana terminu
    case 5:  //Odwolanie wizyty
      //printf("opcja 3,4,5\n");
      strcpy(dzialaniePacjenta.PESEL, logowanie.PESEL);
      printf("%s\n", dzialaniePacjenta.PESEL);
      
      msgsnd(mid, &dzialaniePacjenta, sizeof(dzialaniePacjenta)-sizeof(long), 0); //wysylanie do serwera
      
      msgrcv(id_ipc_pacjent, &komunikat, sizeof(komunikat)-sizeof(long), 3, 0); //komunikat zwrotny
      printf("\n%s\n", komunikat.tresc);
      break;
      
    case 6:
      printf("Wylogowano\n");
      shutdown=1;
      break;
      
    default:
      printf("Bledny numer!\n");
      break;
  }
  }
}


int main(int argc, char* argv[])
{
  komunikat.type=3;
  dzialaniePacjenta.type=5;
  int x=200000;
  int menu;
  int mid; //id kolejki komunikatów serwera
  int id_ipc_pacjent; //id kolejki komunikatów pacjenta
  
  mid = msgget(0x123, 0777 | IPC_CREAT); //serwer
  
  do //wybieranie unikalnego id_ipc_pacjent
  {
    id_ipc_pacjent=msgget(x, 0777 |IPC_CREAT| IPC_EXCL);
    x--;
  }while(id_ipc_pacjent<0);
  logowanie.id=id_ipc_pacjent;
  pacjent.id=id_ipc_pacjent;
  dzialaniePacjenta.id=id_ipc_pacjent;
  
  printf("\nWitaj!\n");
  
  printf("\nCo chcesz zrobić?\n 1 - Logowanie\n 2 - Rejestracja\n 3 - Wyjscie\n\n");
  scanf("%d", &menu);
  
  switch(menu)
  {
    case 1:
      printf("LOGOWANIE\n");
      printf("Podaj PESEL: ");
      scanf("%s", logowanie.PESEL); //przydalo by sie sprawdzac czy jest taki uzytkownik
      printf("Podaj haslo: ");
      scanf("%s" ,logowanie.haslo);
      logowanie.type=2;
      
      msgsnd(mid, &logowanie, sizeof(logowanie)-sizeof(long), 0);
      msgrcv(id_ipc_pacjent, &komunikat, sizeof(komunikat)-sizeof(long), 3, 0);
      printf("\n%s\n", komunikat.tresc);
      if( strcmp("Zalogowano", komunikat.tresc)==0 )
      {
	printf("\nCo chcesz zrobić?\n 1 - Wyświetl listę lekarzy w danym dniu\n 2 - Wyświetl dostępne terminy danego lekarza\n 3- Wyświetl status wizyty\n 4 - Zmiana terminu wizyty\n 5 - Odwołanie wizyty\n 6 - Wyloguj\n");
        menu_po_zalogowaniu(mid, id_ipc_pacjent);
      }
      break;
      
    case 2: 
      printf("REJESTRACJA\n");
      printf("Podaj imie: ");
      scanf("%s" ,pacjent.imie);
      printf("Podaj nazwisko: ");
      scanf("%s" ,pacjent.nazwisko);
      printf("Podaj PESEL: "); //przydało by się sprawdzać czy juz nie ma takiego użytkownika
      scanf("%s" ,pacjent.PESEL);
      printf("Twoje haslo: ");
      scanf("%s" ,pacjent.haslo);
      printf("\n");
      pacjent.type=1;
      
      msgsnd(mid, &pacjent, sizeof(pacjent)-sizeof(long), 0);
      //printf("wyslano\n");
      msgrcv(pacjent.id, &komunikat, sizeof(komunikat)-sizeof(long), 3, 0);
      printf("\n%s\n", komunikat.tresc);
      //printf("odebrano\n");
      
      break;
      
    case 3:
      printf("KONIEC\n"); 
      break;
      
  }
  msgctl(id_ipc_pacjent, IPC_RMID, NULL);
  return 0;
}
