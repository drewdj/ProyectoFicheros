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



