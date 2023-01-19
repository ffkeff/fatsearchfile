#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

extern char SPACE;
extern int SHORT_NAME;
extern struct bpbinfo;
extern struct sfn_record;

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
