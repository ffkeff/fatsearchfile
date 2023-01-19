#pragma once

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

char *parse_name(char *search_name);

void bpb_info(struct bpbinfo *bpb, FILE *stream);

int string_compare(char *string1, char *string2);

struct sfn_record search_file_by_name(char *search_name, int root_offset, uint16_t root_records, FILE *stream);

uint16_t *cluster_chain(int fat_offset, uint8_t first_cluster, FILE *stream);

uint8_t *read_cluster(int offset, int cluster_size, FILE *stream);

void write_to_file(uint8_t *data, int size, FILE *stream);
