// =============== LEKARZ ===============

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

//type=4
struct msgLekarz{
  long type;
  int nr_lekarza;
  int dzialanie; //1-przegląd pacjentów, 2-urlop
  int poczatek; //pierwszy dzien urlopu
  int koniec; // ostatni dzien urlopu
  int id; //id kolejki
} lekarz;

struct msgOdpowiedz{
  long type;
  char tresc[1500];
} komunikat;

int main(int argc, char* argv[])
{
  int x=200000;
  int mid;
  int id_ipc_lekarz;
  int kto;
  int menu;
  
  
  mid = msgget(0x123, 0777 | IPC_CREAT); //serwer
  
  do //wybieranie unikalnego id_ipc_lekarz;
  {
    id_ipc_lekarz=msgget(x, 0777 |IPC_CREAT| IPC_EXCL);
    x--;
  }while(id_ipc_lekarz<0);
  
  lekarz.type=4;
  lekarz.id=id_ipc_lekarz;
  
  printf("\nWitaj!\n");
  printf("Zaloguj się jako:\n1 - Doktor House \n2 - Doktor Watson\n");
  scanf("%d", &kto);
  
  if((kto==1)||(kto==2))
  {
    if(kto==1) lekarz.nr_lekarza=1; //House
      else lekarz.nr_lekarza=2; //Watson
      
      printf("\nCo chcesz zrobić?\n1 - Wyświetl swoich pacjentów\n2 - Weź urlop\n");
      scanf("%d", &menu);
    
    switch(menu)
    {
      case 1: //wyswietlanie pacjentów
	lekarz.dzialanie=1;
	msgsnd(mid, &lekarz, sizeof(lekarz)-sizeof(long), 0);
	msgrcv(id_ipc_lekarz, &komunikat, sizeof(komunikat)-sizeof(long), 3, 0);
	printf("\n%s\n", komunikat.tresc);
	break;
	
      case 2: //urlop
	lekarz.dzialanie=2;
	printf("Od kiedy: ");
	scanf("%d", &lekarz.poczatek);
	printf("\nDo kiedy: ");
	scanf("%d", &lekarz.koniec);
	printf("\n");
	msgsnd(mid, &lekarz, sizeof(lekarz)-sizeof(long), 0);
	break;
	
      case 3:
	printf("Błąd!\n");
	break; 
    }
      
  }
  else printf("Błąd!\n");
	     
  
  msgctl(id_ipc_lekarz, IPC_RMID, NULL);
  return 0;
}