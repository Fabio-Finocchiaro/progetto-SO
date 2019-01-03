#include<stdio.h>
#include<stdlib.h>
#include<sys/sem.h>
#include<sys/ipc.h>
#include<sys/types.h>
#include<sys/shm.h>
#include<unistd.h>
#include<string.h>
#include<dirent.h>
#include<sys/sysinfo.h>
#include<sys/stat.h>
#include<pthread.h>
#define N 200

//creo la lista
typedef struct lista{
char nome[64];
int tipo;
char path[200];
}TipoElemLista;

typedef struct nodolista{
TipoElemLista info;
struct nodolista *next;
}Tiponodolista;

typedef Tiponodolista* Tipolista;

int controlloterminazione[N];

Tipolista lis = NULL;
pthread_mutex_t mutex_lista;
pthread_mutex_t mutex2_lista;
void *funzione_thread(void*arg);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void inserimentoordinatoinlista(Tipolista *lis, char* elem, int elem2, char* path){
Tipolista paux, prec, cor;
char pathaux[200];
if((paux = (Tipolista)malloc(sizeof(Tiponodolista))) == NULL){
perror("ERRORE ALLOCAZIONE MEMORIA");
exit(1);
}

strcpy(paux->info.nome, elem);
paux->info.tipo = elem2;
strcpy(pathaux, path);
strcat(pathaux,"/");
strcat(pathaux, paux->info.nome);
strcpy(paux->info.path, pathaux);
cor = *lis;
prec = NULL;
while(cor != NULL && strcmp(cor->info.nome, elem) < 0){
prec = cor;
cor = cor->next;
}

paux->next = cor;

if(prec != NULL)
prec->next = paux;
else
*lis=paux;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void visitalista(Tipolista lis){
while(lis != NULL){
//printf("%s \n", lis->info.nome);
if (lis->info.tipo == 1){
printf("Directory\n");
}
printf("%s\n", lis->info.path);
lis = lis->next;
}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void scansionalista(Tipolista *lis){


}
//questa funzione mi permette di restituirmi il puntatore alla directory in modo da poterlo utilizzare
Tipolista trovaDir(Tipolista lis){
while(lis!= NULL){
if (lis->info.tipo==1){
return lis;
}else
lis = lis->next;
}
return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void eliminadirectory(Tipolista* lis, TipoElemLista elem){
Tipolista paux;
if((*lis) != NULL)
if(strcmp((*lis)->info.path, elem.path)==0){
printf("%s, ", elem.path);
paux = (*lis);
(*lis) = (*lis)->next;
free(paux);
printf("tolto\n");
}
else
eliminadirectory(&(*lis)->next, elem);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(void){

//utilizzo una opendir e una readdir
//con opendir apre la directory passata da tastiera e salvata in una stringa e ritorna un puntatore allo stream della directory, questa funzione permette un list di file.

int Ncores;
int i;
int res;
pthread_t th;
int tipo;
int thread;
int risultato;

pthread_mutex_init(&mutex_lista, NULL);
pthread_mutex_init(&mutex2_lista, NULL);

for(i=0; i<N; i++){
controlloterminazione[i]= 0;
}
struct dirent *de;
//apre la direcory corrente dove mi trovo e la assegna ad un puntatore
DIR *dr;
char path[N];
printf("inserisci il path assoluto: \n");
scanf("%s", path);
dr = opendir(path);
//tramite readdir stampo i vari campi della lista essendo un while
while((de = readdir(dr)) != NULL){
//salvo l'elemento in una lista e incremento la lista
if(de->d_type == DT_DIR){
tipo = 1;
}else{
tipo = 0;
}
if(strncmp(de->d_name, ".", 1)!=0){
pthread_mutex_lock(&mutex_lista);
inserimentoordinatoinlista(&lis, de->d_name, tipo, path);
pthread_mutex_unlock(&mutex_lista);
}
}
visitalista(lis);
printf("INSERIMENTO EFFETTUATO!\n");
closedir(dr);

//così vedo il numero di core
Ncores = get_nprocs_conf();
printf("%d\n", Ncores);

for(i=0;i<Ncores;i++){
res = pthread_create(&th, NULL, funzione_thread, (void *)(intptr_t)i);
if (res != 0){
perror("CREAZIONE FALLITA\n");
exit(EXIT_FAILURE);
}//if

}//for
//for(i=0; i<Ncores; i++){
if((pthread_join(th, NULL)) != 0){
perror("Errore");
exit(EXIT_FAILURE);
//}
}
printf("//////////////////////////////////////////////\n");
visitalista(lis);
pthread_mutex_destroy(&mutex_lista);
pthread_mutex_destroy(&mutex2_lista);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void* funzione_thread(void* arg){
int numcores = (intptr_t)arg;
int finito = 0;
Tipolista Esistedirectory;
DIR *dr;
int tipo;
struct dirent *de;
char pathaux[200];
int i = 0;
//faccio scorrere la lista e trovo la directory
while(!finito){
pthread_mutex_lock(&mutex_lista);
Esistedirectory = trovaDir(lis);
pthread_mutex_unlock(&mutex_lista);
if(Esistedirectory != NULL){
Esistedirectory->next = NULL;
pthread_mutex_lock(&mutex_lista);
eliminadirectory(&lis, Esistedirectory->info);
pthread_mutex_unlock(&mutex_lista);
strcpy(pathaux,Esistedirectory->info.path);
pthread_mutex_lock(& mutex2_lista);
controlloterminazione[numcores] = 0;
pthread_mutex_unlock(& mutex2_lista);
dr = opendir(pathaux);
pthread_mutex_lock(&mutex_lista);
while((de = readdir(dr)) != NULL){
//salvo l'elemento in una lista e incremento la lista

if(de->d_type == DT_DIR){
tipo = 1;
if(strncmp(de->d_name, ".", 1)!=0){
inserimentoordinatoinlista(&lis, de->d_name, tipo, pathaux);
}
}else{
tipo = 0;
if(strncmp(de->d_name, ".", 1)!=0){
inserimentoordinatoinlista(&lis, de->d_name, tipo, pathaux);
}//if
}

}//while
pthread_mutex_unlock(&mutex_lista);
}//if
finito = 1;
}//while
}
