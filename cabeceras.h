#include<stdio.h>
#include<string.h>
#include<ctype.h>
#define SIZE_BLOQUE 512
#define MAX_INODOS 24
#define MAX_FICHEROS 20
#define MAX_BLOQUES_DATOS 96
#define PRIM_BLOQUE_DATOS 4
#define MAX_BLOQUES_PARTICION MAX_BLOQUES_DATOS+PRIM_BLOQUE_DATOS
//superbloque + bytemap inodos y bytemap bloques + inodos + directorio
#define MAX_NUMS_BLOQUE_INODO 7
#define LEN_NFICH 17
#define NULL_INODO 0xFFFF
#define NULL_BLOQUE 0xFFFF

/* Estructura del superbloque */
typedef struct {
    unsigned int s_inodes_count;          /* inodos de la partición */
    unsigned int s_blocks_count;          /* bloques de la partición */
    unsigned int s_free_blocks_count;     /* bloques libres */
    unsigned int s_free_inodes_count;     /* inodos libres */
    unsigned int s_first_data_block;      /* primer bloque de datos */
    unsigned int s_block_size;        /* tamaño del bloque en bytes */
    unsigned char s_relleno[SIZE_BLOQUE-6*sizeof(unsigned int)]; /* relleno a 0's*/
} EXT_SIMPLE_SUPERBLOCK;

/* Bytemaps, caben en un bloque */
typedef struct {
    unsigned char bmap_bloques[MAX_BLOQUES_PARTICION];
    unsigned char bmap_inodos[MAX_INODOS];  /* inodos 0 y 1 reservados, inodo 2 directorio */
    unsigned char bmap_relleno[SIZE_BLOQUE-(MAX_BLOQUES_PARTICION+MAX_INODOS)*sizeof(char)];
} EXT_BYTE_MAPS;

/* inodo */
typedef struct {
    unsigned int size_fichero;
    unsigned short int i_nbloque[MAX_NUMS_BLOQUE_INODO];
} EXT_SIMPLE_INODE;

/* Lista de inodos, caben en un bloque */
typedef struct {
    EXT_SIMPLE_INODE blq_inodos[MAX_INODOS];
    unsigned char blq_relleno[SIZE_BLOQUE-MAX_INODOS*sizeof(EXT_SIMPLE_INODE)];
} EXT_BLQ_INODOS;

/* Entrada individual del directorio */
typedef struct {
    char dir_nfich[LEN_NFICH];
    unsigned short int dir_inodo;
} EXT_ENTRADA_DIR;

/* Bloque de datos */
typedef struct{
    unsigned char dato[SIZE_BLOQUE];
} EXT_DATOS;

int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2){
   //Flagas para saber si existen los tres argumentos
   int flagArg1 = 0;
   int flagArg2 = 0;
   int flagArg3 = 0;
   //Array para saber donde estan los espacios, para colocar '\0'
   int positionOfSpace[] = {0, 0, 0};
   int sizeOfOrden = 0;
   int sizeOfArgumento1 = 0;
   int sizeOfArgumento2 = 0;
   orden[0] = '\0';
   argumento1[0] = '\0';
   argumento2[0] = '\0';
   int sizeOfComando = 0;
   int numberOfSpace = 0;
   //Contar el numero de espacios y guardar su posicion
   while(strcomando[sizeOfComando] != '\0'){
      if(strcomando[sizeOfComando] == ' '){
         positionOfSpace[numberOfSpace] = sizeOfComando;
         numberOfSpace++;
      }
      sizeOfComando++;
   }
   int flag = 0;
    //Bucle para meter caracter a caracter Orden Argumento1 y Argumento2
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
   //Si el ultimo caracter de algun argumento es un \n cambiarlo por un '\0'
   if(argumento2[strlen(argumento2) - 1] == '\n'){
      argumento2[strlen(argumento2) - 1] = '\0';
   }
   if(orden[strlen(orden) - 1] == '\n'){
      orden[strlen(orden) - 1] = '\0';
   }
   if(argumento1[strlen(argumento1) - 1] == '\n'){
      argumento1[strlen(argumento1) - 1] = '\0';
   }
   //Comprobar el numero de argumentos y devolver el resultado de la comprobacion
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

int BuscaFich(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombre){
    for (int i = 0; i < MAX_FICHEROS; ++i) {
        int stringsize=0;
        int comparador=0;
        int k=0;
        int dirInodo = directorio[i].dir_inodo;
        if (0<dirInodo&&dirInodo<24){//Mismo bucle de busqueda que en renombrar para encontrar el directorio deseado
            if (inodos->blq_inodos[dirInodo].size_fichero!=0){
                while (directorio[i].dir_nfich[k]!='\0'){
                    stringsize++;
                    if (directorio[i].dir_nfich[k]==nombre[k]){
                        comparador++;
                    }
                    k++;
                }
                if (comparador==stringsize){//Una vez encontrado se borra todo su rastro
                    return i;
                }

            }
        }
    }
    return '\0';
}

//A continuacion hay 4 funciones para grabar los cambios, que buscan con fseek donde se deben de colocar los punteros para empezar a escribir los bloques
//y posteriormente sobreescriben lo que estan en esa posicion dentro del archivo fich
void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich){
    fseek(fich, SIZE_BLOQUE * 2, SEEK_SET);
    fwrite(inodos, SIZE_BLOQUE, 1, fich);
    fseek(fich, SIZE_BLOQUE * 3, SEEK_SET);
    fwrite(directorio, SIZE_BLOQUE, 1, fich);
}

void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich){
    fseek(fich, SIZE_BLOQUE, SEEK_SET);
    fwrite(ext_bytemaps, SIZE_BLOQUE, 1, fich);
}

void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich){
    fseek(fich, 0, SEEK_SET);
    fwrite(ext_superblock, SIZE_BLOQUE, 1, fich);
}

void GrabarDatos(EXT_DATOS *memdatos, FILE *fich){
    fseek(fich, SIZE_BLOQUE * 4, SEEK_SET);
    fwrite(memdatos->dato, SIZE_BLOQUE, MAX_BLOQUES_DATOS, fich);
}

void info(EXT_SIMPLE_SUPERBLOCK *extSimpleSuperblock){
    printf("Bloque %d Bytes\n",extSimpleSuperblock->s_block_size);
    printf("inodos particion = %d\n",extSimpleSuperblock->s_inodes_count);
    printf("inodos libres = %d\n",extSimpleSuperblock->s_free_inodes_count);
    printf("Bloques particion = %d\n",extSimpleSuperblock->s_blocks_count);
    printf("Bloques libres = %d\n",extSimpleSuperblock->s_free_blocks_count);
    printf("Primer bloque de datos = %d\n",extSimpleSuperblock->s_first_data_block);
}

void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps){
    printf("\nInodos:");
    for (int k = 0; k <  sizeof(ext_bytemaps->bmap_inodos); ++k) {
        printf(" %d",ext_bytemaps->bmap_inodos[k]);
    }

    printf("\nBloques [0-25] :");
    for (int k = 0; k < 25; ++k) {
        printf(" %d",ext_bytemaps->bmap_bloques[k]);
    }
}

void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos){
    for (int k = 0; k < MAX_FICHEROS; ++k) {
        int p = 0;
        int dirInodo = directorio[k].dir_inodo;
        if (0<=dirInodo&&dirInodo<=24){//Primero se filtra la direccion de inodo para que entre en el tamaño
            if (inodos->blq_inodos[dirInodo].size_fichero!=0){//Si el tamaño del fichero es diferente de 0 (no esta vacio) se printea la informacion necesaria
                printf("\n");
                while (directorio[k].dir_nfich[p]!='\0'){
                    printf("%c",directorio[k].dir_nfich[p]);
                    p++;
                }
                printf("    tamano:%d",inodos->blq_inodos[directorio[k].dir_inodo].size_fichero);
                printf("    inodo:%d",directorio[k].dir_inodo);
                printf("    bloques:");
                for (int l = 0; l < MAX_NUMS_BLOQUE_INODO; ++l) {
                    if (inodos->blq_inodos[directorio[k].dir_inodo].i_nbloque[l]!=NULL_BLOQUE&&inodos->blq_inodos[directorio[k].dir_inodo].i_nbloque[l]!=0){
                        printf(" %d",inodos->blq_inodos[directorio[k].dir_inodo].i_nbloque[l]);
                    }
                }
            }
        }

    }

}

int Renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombreantiguo, char *nombrenuevo){
    int i = BuscaFich(directorio,inodos,nombreantiguo );
    if (i!='\0'){
        int p=0;
        while (nombrenuevo[p]!='\0'){
            directorio[i].dir_nfich[p]=nombrenuevo[p];
            p++;
        }
        directorio[i].dir_nfich[p]='\0';
    } else
        printf("Directorio no encontrado.");
    return 0;
}

int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre){
    int i = BuscaFich(directorio,inodos,nombre);
    if (i!='\0'){
        for (int l = 0; l < sizeof(inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque); ++l) {
            if (inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[l]!=NULL_BLOQUE&&inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[l]!=0){
                printf("%s",memdatos[inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[l]-4]);
            }
        }

    } else
        printf("Directorio no encontrado.");
    return 0;
}

int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre,  FILE *fich){
    int i = BuscaFich(directorio,inodos,nombre);
    if (i!='\0'){
        ext_bytemaps->bmap_inodos[directorio[i].dir_inodo]=0;
        for (int l = 0; l < sizeof(inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque); ++l) {
            if (inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[l]!=NULL_BLOQUE&&inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[l]!=0){
                ext_bytemaps->bmap_bloques[inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[l]]=0;
            }
        }
        inodos->blq_inodos[directorio[i].dir_inodo].size_fichero=0;
        for (int j = 0; j < MAX_NUMS_BLOQUE_INODO; ++j) {
            inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[j]=NULL_BLOQUE;
        }
        directorio[i].dir_inodo=0xFFFF;
        directorio[i].dir_nfich[0]='\0';
    } else
        printf("Directorio no encontrado.");

    return 0;
}

int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino,  FILE *fich){
    int i = BuscaFich(directorio,inodos,nombreorigen);
    if (i!='\0'){
        if (BuscaFich(directorio,inodos,nombredestino) != '\0'){
            printf("\nEl directorio ya existe.");
            return 0;
        }
        int p = 0;
        for (int j = 0; j < MAX_INODOS; ++j) {
            if (ext_bytemaps->bmap_inodos[j]==0){
                ext_bytemaps->bmap_inodos[j]=1;//marca el bytemap
                inodos->blq_inodos[j].size_fichero=inodos->blq_inodos[directorio[i].dir_inodo].size_fichero;//asigna mismo tamaño que origen
                for (int l = 0; l < MAX_NUMS_BLOQUE_INODO; ++l) {
                    if (inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[l]!=65535&&inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[l]!=0){
                        for (int m = 0; m < MAX_BLOQUES_DATOS; ++m) {//asignar bloques
                            if (ext_bytemaps->bmap_bloques[m]==0){
                                ext_bytemaps->bmap_bloques[m]=1;
                                memdatos[m-4]=memdatos[l-4];
                                inodos->blq_inodos[j].i_nbloque[p]=m;
                                p++;
                                break;

                            }
                        }
                    }
                }
                for (int l = 0; l < MAX_FICHEROS; ++l) {
                    if (directorio[l].dir_inodo==0xFFFF){
                        int m=0;
                        for (; m < strlen(nombredestino); ++m) {
                            directorio[l].dir_nfich[m]=nombredestino[m];
                        }
                        directorio[l].dir_nfich[m]='\0';
                        directorio[l].dir_inodo=j;
                        break;
                    }
                }
                break;
            }
        }
    } else
        printf("Directorio no encontrado.");
    return 0;
}



