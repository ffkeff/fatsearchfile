#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

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
