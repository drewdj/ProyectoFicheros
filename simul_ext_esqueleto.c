#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include "cabeceras.h"

#define LONGITUD_COMANDO 100

void info(EXT_SIMPLE_SUPERBLOCK *extSimpleSuperblock);
void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps);
int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2);
void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup);
int BuscaFich(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, 
              char *nombre);
void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos);
int Renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, 
              char *nombreantiguo, char *nombrenuevo);
int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, 
             EXT_DATOS *memdatos, char *nombre);
int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos,
           EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock,
           char *nombre,  FILE *fich);
int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos,
           EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock,
           EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino,  FILE *fich);
//void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich);
//void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich);
//void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich);
//void GrabarDatos(EXT_DATOS *memdatos, FILE *fich);

int main()
{
	 char comando[LONGITUD_COMANDO];
	 char orden[LONGITUD_COMANDO];
	 char argumento1[LONGITUD_COMANDO];
	 char argumento2[LONGITUD_COMANDO];

    char *ordenHeap;
    char *argumento1Heap;
    char *argumento2Heap;
	 
	 int i,j;
	 unsigned long int m;
     EXT_SIMPLE_SUPERBLOCK ext_superblock;
     EXT_BYTE_MAPS ext_bytemaps;
     EXT_BLQ_INODOS ext_blq_inodos;
     EXT_ENTRADA_DIR directorio[MAX_FICHEROS];
     EXT_DATOS memdatos[MAX_BLOQUES_DATOS];
     EXT_DATOS datosfich[MAX_BLOQUES_PARTICION];
     int entradadir;
     int grabardatos;
     FILE *fent;
     
     // Lectura del fichero completo de una sola vez
     
     fent = fopen("D:\\Users\\Documents\\GitHub\\ProyectoFicheros\\particion.bin","r+b");
     fread(&datosfich, SIZE_BLOQUE, MAX_BLOQUES_PARTICION, fent);    
     
     
     memcpy(&ext_superblock,(EXT_SIMPLE_SUPERBLOCK *)&datosfich[0], SIZE_BLOQUE);
     memcpy(&directorio,(EXT_ENTRADA_DIR *)&datosfich[3], SIZE_BLOQUE);
     memcpy(&ext_bytemaps,(EXT_BLQ_INODOS *)&datosfich[1], SIZE_BLOQUE);
     memcpy(&ext_blq_inodos,(EXT_BLQ_INODOS *)&datosfich[2], SIZE_BLOQUE);
     memcpy(&memdatos,(EXT_DATOS *)&datosfich[4],MAX_BLOQUES_DATOS*SIZE_BLOQUE);
     // Bucle de tratamiento de comandos
     for (;;){
		 do {
		 printf ("\n>> ");
		 fflush(stdin);
       fgets(comando, LONGITUD_COMANDO, stdin);
		} while (ComprobarComando(comando,orden,argumento1,argumento2) != 0);
	      if (strcmp(orden,"dir")==0) {
            Directorio(directorio,&ext_blq_inodos);
            continue;
         }
         // Escritura de metadatos en comandos rename, remove, copy
         else if (strcmp(orden,"info") == 0){
             info(&ext_superblock);
         }
         else if(strcmp(orden,"bytemaps") == 0){
             Printbytemaps(&ext_bytemaps);
         }
         else if(strcmp(orden,"rename") == 0){
             Renombrar(directorio,&ext_blq_inodos,argumento1,argumento2);
            /*Grabarinodosydirectorio(&directorio,&ext_blq_inodos,fent);
            GrabarByteMaps(&ext_bytemaps,fent);
            GrabarSuperBloque(&ext_superblock,fent);
            if (grabardatos)
               GrabarDatos(&memdatos,fent);
            grabardatos = 0;*/
         }
         else if(strcmp(orden,"imprimir") == 0){
             Imprimir(directorio,&ext_blq_inodos,datosfich,argumento1);
         }
         else if(strcmp(orden,"remove") == 0){
             Borrar(directorio,&ext_blq_inodos,&ext_bytemaps,&ext_superblock,argumento1,fent);
            /*Grabarinodosydirectorio(&directorio,&ext_blq_inodos,fent);
            GrabarByteMaps(&ext_bytemaps,fent);
            GrabarSuperBloque(&ext_superblock,fent);
            if (grabardatos)
               GrabarDatos(&memdatos,fent);
            grabardatos = 0;*/
         }
         else if(strcmp(orden,"copy") == 0){
            Copiar(directorio,&ext_blq_inodos,&ext_bytemaps,&ext_superblock,memdatos,argumento1,argumento2,fent);
            /*Grabarinodosydirectorio(&directorio,&ext_blq_inodos,fent);
            GrabarByteMaps(&ext_bytemaps,fent);
            GrabarSuperBloque(&ext_superblock,fent);
            if (grabardatos)
               GrabarDatos(&memdatos,fent);
            grabardatos = 0;*/
         }   
         //Si el comando es salir se habrán escrito todos los metadatos
         //faltan los datos y cerrar
         else if (strcmp(orden,"salir")==0){
            //GrabarDatos(&memdatos,fent);
            fclose(fent);
            return 0;
         } else{
             printf("ERROR: Comando ilegal [bytemaps,copy,dir,info,imprimir,rename,remove,salir]\n");
         }
     }
}
int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2){
   int flagArg1 = 0;
   int flagArg2 = 0;
   int flagArg3 = 0;
   int positionOfSpace[] = {0, 0, 0};
   int sizeOfOrden = 0;
   int sizeOfArgumento1 = 0;
   int sizeOfArgumento2 = 0;
   orden[0] = '\0';
   argumento1[0] = '\0';
   argumento2[0] = '\0';
   int sizeOfComando = 0;
   int numberOfSpace = 0;
   while(strcomando[sizeOfComando] != '\0'){
      if(strcomando[sizeOfComando] == ' '){
         positionOfSpace[numberOfSpace] = sizeOfComando;
         numberOfSpace++;
      }
      sizeOfComando++;
   }
   int flag = 0;
/*
   for(int i = 0; i < sizeOfComando; i++){
      if(i < positionOfSpace[0] && numberOfSpace == 0){
         sizeOfOrden++;
      }
      if(i < positionOfSpace[1] && i > positionOfSpace[0] && numberOfSpace == 1){
         sizeOfArgumento1++;
      }
      if(i < positionOfSpace[2] && i > positionOfSpace[1] && numberOfSpace == 2){
         sizeOfArgumento2++;
      }
   }*/

   

   for(int i = 0; i < sizeOfComando; i++){
      char filler = strcomando[i];

      if(flag == 0){
         sizeOfOrden++;
      }
      if(flag == 1){
         sizeOfArgumento1++;
      }
      if(flag == 2){
         sizeOfArgumento2++;
      }
      if(filler == ' ' && flag == 0){
         flag = 1;
         continue;
      }
      else if(filler == ' ' && flag == 1){
         flag = 2;
         continue;
      }
      else if(flag == 0){
         strncat(orden, &filler, 1);
      }
      else if(flag == 1){
         strncat(argumento1, &filler, 1);
      }
      else if(flag == 2){
         strncat(argumento2, &filler, 1);
      }
      else if(filler == '\0'){
         break;
      }
      else{
         break;
      }
   }
   if(argumento2[strlen(argumento2) - 1] == '\n'){
      argumento2[strlen(argumento2) - 1] = '\0';
   }
   if(orden[strlen(orden) - 1] == '\n'){
      orden[strlen(orden) - 1] = '\0';
   }
   if(argumento1[strlen(argumento1) - 1] == '\n'){
      argumento1[strlen(argumento1) - 1] = '\0';
   }
   if(*orden != '\0'){
      flagArg1 = 1;
   }
   if(*argumento1 != '\0'){
      flagArg2 = 1;
   }
   if(*argumento2 != '\0'){
      flagArg3 = 1;
   }
   if(flagArg1 == 1 && numberOfSpace == 0){
      return 0;
   }
   if(flagArg1 == 1 && flagArg2 == 1 && numberOfSpace == 1){
      return 0;
   }
   if(flagArg1 == 1 && flagArg2 == 1 && flagArg3 == 1){
      return 0;
   }
   else {
      return 1;
   }
}
