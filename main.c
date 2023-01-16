#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#define SPACE 0x20
#define SHORT_NAME 11
#define MAX 8196
#define FILE_NAME "fatmd.bin"

#pragma pack(push, 1)
struct bpbinfo{ 
 
    uint8_t jmp[3];
    uint64_t manufacturer;
    uint16_t bytes_in_sector;
    uint8_t sectors_in_claster;
    uint16_t reserved_sectors;
    uint8_t fat_tables_count;
    uint16_t root_records;
    uint16_t sectors_in_partition;
    uint8_t device_type;
    uint16_t fat_table_sectors_size;
    uint16_t sectors_in_track;
    uint16_t working_surfaces_count;
    uint32_t hidden_sectors_count;
    uint32_t long_sectors_in_partition;
};

struct sfn_record{ 
  
    char dir_name[SHORT_NAME];
    uint8_t dir_attr;
    uint8_t dir_ntres;
    uint8_t crt_time_tenth;
    uint16_t dir_crt_time;
    uint16_t dir_crt_date;
    uint16_t dir_lst_acc_date;
    uint16_t fst_clus_hi;
    uint16_t wrt_time;
    uint16_t wrt_date;
    uint16_t dir_fst_clus_lo;
    uint32_t dir_file_size;
};

#pragma pack(pop)

char *parse_name(char *search_name){
 
    char *buf = (char*)malloc(SHORT_NAME * sizeof(char));
    int x;
    for(x = 0; x != strlen(search_name); x++)
        buf[x] = search_name[x];
    
    for(; x != SHORT_NAME; x++)
        buf[x] = SPACE;
    
    return buf;
}

void bpb_info(struct bpbinfo *bpb, FILE *stream){
 
    if (NULL == stream){
        fprintf(stderr, "stream is null\n");
        exit(EXIT_FAILURE);
    }
    
    int fseek_ret = fseek(stream, 0, SEEK_SET);
    if (-1 == fseek_ret){
        fprintf(stderr, "fseek() failed\n");
        exit(EXIT_FAILURE);
    }
    
    size_t fread_ret = fread(bpb, sizeof(struct bpbinfo), 1, stream);
    if (1 != fread_ret){
        fprintf(stderr, "fread() failed\n");
        exit(EXIT_FAILURE);
    }
}

int string_compare(char *string1, char *string2){
    
    for(int x = 0; x != SHORT_NAME; x++)
        if (string1[x] != string2[x])
            return -1;
    
    return 0;
}

struct sfn_record search_file_by_name(char *search_name, int root_offset, uint16_t root_records, FILE *stream){
 
    if (NULL == stream){
        fprintf(stderr, "stream is null\n");
        exit(EXIT_FAILURE);
    }
    
    int fseek_ret = fseek(stream, root_offset, SEEK_SET);
    if (-1 == fseek_ret){
        fprintf(stderr, "fseek() failed\n");
        exit(EXIT_FAILURE);
    }
    
    struct sfn_record *search_file;
    search_file = (struct sfn_record*)malloc(root_records * 32);
    if (NULL == search_file){
        fprintf(stderr, "malloc() failed\nmemory allocated\n");
        exit(EXIT_FAILURE);
    }
    
    size_t fread_ret = fread(search_file, sizeof(struct sfn_record), root_records, stream);
    if (root_records != fread_ret){
        fprintf(stderr, "fread() failed\n");
        exit(EXIT_FAILURE);
    }

    for(int x = 0; x != root_records; x++)
        if (0 == string_compare(search_file[x].dir_name, search_name))
            return search_file[x];
    
    fprintf(stderr,"file is missing\n");
    exit(EXIT_FAILURE);
}

uint16_t *cluster_chain(int fat_offset, uint8_t first_cluster, FILE *stream){
    
    uint16_t *c_chain = NULL;
    c_chain = (uint16_t*)malloc(sizeof(uint16_t) * 64);
    if (NULL == c_chain){
        fprintf(stderr, "malloc() failed\nmemory allocated\n");
        exit(EXIT_FAILURE);
    }
    c_chain[0] = first_cluster;
    uint8_t *c_buf = NULL;
    c_buf = (uint8_t*)malloc(sizeof(uint8_t) * 3);
    if (NULL == c_buf){
        fprintf(stderr, "malloc() failed\nmemory allocated\n");
        exit(EXIT_FAILURE);
    }
    
    int fseek_ret = fseek(stream, fat_offset + ((first_cluster * 3) / 2), SEEK_SET);
    if (-1 == fseek_ret){
        fprintf(stderr, "fseek() failed\n");
        exit(EXIT_FAILURE);
    }
    
    int x = 0;
    size_t fread_ret;
    while(0xfff != c_chain[x]){ 
        fread_ret = fread(c_buf, 3, 1, stream);
        if (1 != fread_ret){
            fprintf(stderr, "fread() failed\n"); 
            exit(EXIT_FAILURE);
        }
        x++;
        c_chain[x] = (c_buf[1] << 4) << 8;
        c_chain[x] += c_buf[0];
        if (0xf0ff == c_chain[x]) // 0xf0ff костыль, компилятор считает что мое бесзнаковое число все же со знаком, поэтому оно дальше в алгоритме затирается поэтому число смещено на 8, а не на 4
            break;
        printf("c_chain: %i\n", c_chain[x]);
        x++;
        c_chain[x] = ((c_buf[2]) << 4) + (c_buf[1] >> 4);
        
        printf("c_chain: %i\n", c_chain[x]);
       
        fseek_ret = fseek(stream, fat_offset + ((c_chain[x] * 3) / 2), SEEK_SET);
        if (-1 == fseek_ret){
            fprintf(stderr, "fseek() failed\n");
            exit(EXIT_FAILURE);
        }
    }
    
    return c_chain;
}

uint8_t *read_cluster(int offset, int cluster_size, FILE *stream){
    
    int fseek_ret = fseek(stream, offset, SEEK_SET);
    if (-1 == fseek_ret){
        fprintf(stderr, "fseek() failed\n");
        exit(EXIT_FAILURE);
    }
    
    uint8_t *c_buf = NULL;
    c_buf = (uint8_t*)malloc(sizeof(uint8_t) * cluster_size);
    if (NULL == c_buf){
        fprintf(stderr, "malloc() failed\nmemory allocated\n");
        exit(EXIT_FAILURE);
    }
    
    size_t ret;
    ret = fread(c_buf, sizeof(uint8_t), cluster_size, stream);
    if (cluster_size != ret){
        fprintf(stderr, "fread() failed\n");
        exit(EXIT_FAILURE);
    }
 
    return c_buf;
}

void write_to_file(uint8_t *data, int size, FILE *stream){
    
    size_t ret;
    ret = fwrite(data, sizeof(char), size, stream);
    if (size != ret){
        fprintf(stderr, "fwrite() failed\n");
        exit(EXIT_FAILURE);
    }
}


int main(int argc, char *argv[], char *env[]){
 
    if (argc != 3){
        fprintf(stderr, "args!\n");
        return EXIT_FAILURE;
    }
    
    char *name = parse_name(argv[2]);
    
    FILE *stream = fopen(argv[1], "r");
    
    fprintf(stdout, "start reading bpb... ");
    struct bpbinfo bpb = {0};
    bpb_info(&bpb, stream);
    fprintf(stdout, "done\n");
    printf("bytes in sec:%i\t sec in cl: %i\t res: %i\n", bpb.bytes_in_sector, bpb.sectors_in_claster, bpb.reserved_sectors);
    printf("root offset: %i\n", (bpb.bytes_in_sector * bpb.reserved_sectors) + 
                                (bpb.fat_table_sectors_size * bpb.fat_tables_count * bpb.bytes_in_sector));
    
    fprintf(stdout, "start search sfn record by file name... ");
    struct sfn_record search_record = search_file_by_name(
        name, 
        (bpb.bytes_in_sector * bpb.reserved_sectors) + 
        (bpb.fat_table_sectors_size * bpb.fat_tables_count * bpb.bytes_in_sector), 
        bpb.root_records, 
        stream
    );
    fprintf(stdout, "done\n");
    printf("first cluster: %i\n", search_record.dir_fst_clus_lo);
    fprintf(stdout, "start buid clusters chain... \n");
    uint16_t *file_cluster_chain = cluster_chain(
        bpb.bytes_in_sector * bpb.reserved_sectors,
        search_record.dir_fst_clus_lo,
        stream
    );
    fprintf(stdout, "done\n");
    
    fprintf(stdout, "start reading clusters... \n");
    uint8_t *filedata = NULL;
    filedata = (uint8_t*)malloc(search_record.dir_file_size);
    if (NULL == filedata){
        fprintf(stderr, "malloc() failed\nmemory allocated\n");
        exit(EXIT_FAILURE);
    }
    FILE *fout = fopen(FILE_NAME, "w");
    if (NULL == fout){
        fprintf(stderr, "fopen() failed\n");
        return EXIT_FAILURE;
    }
#pragma warn(disable:37)
    int x;
    for(x = 0; x != search_record.dir_file_size / (bpb.bytes_in_sector * bpb.sectors_in_claster); x++)
        write_to_file(
            read_cluster(
                (bpb.bytes_in_sector * bpb.reserved_sectors) + 
                (bpb.fat_table_sectors_size * bpb.fat_tables_count * bpb.bytes_in_sector) +
                (bpb.root_records * 32) +
                (bpb.bytes_in_sector * bpb.sectors_in_claster * (file_cluster_chain[x] - 2)),
                bpb.sectors_in_claster * bpb.bytes_in_sector,
                stream
            ),
            bpb.bytes_in_sector * bpb.sectors_in_claster,
            fout
        );
    
    if (0 != search_record.dir_file_size % (bpb.bytes_in_sector * bpb.sectors_in_claster))
        write_to_file(
            read_cluster(
                (bpb.bytes_in_sector * bpb.reserved_sectors) + 
                (bpb.fat_table_sectors_size * bpb.fat_tables_count * bpb.bytes_in_sector) +
                (bpb.root_records * 32) +
                (bpb.bytes_in_sector * bpb.sectors_in_claster * (file_cluster_chain[x] - 2)),
                bpb.sectors_in_claster * bpb.bytes_in_sector,
                stream
            ),
            search_record.dir_file_size % (bpb.bytes_in_sector * bpb.sectors_in_claster),
            fout
        );
    fprintf(stdout, "done\n");
    fclose(stream);
    fclose(fout);
    
    return 0;
}
