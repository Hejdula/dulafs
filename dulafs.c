#include <errno.h>
#include <stdio.h>
#include "dulafs.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define I_NODE_RATIO 0.1 
#define CLUSTER_SIZE 1024 //bytes

const int ID_ITEM_FREE = 0;

void setBit(int i, uint8_t* bitset){
    int byte_index = i/8;
    int bit_offset = i%8;
    bitset[byte_index] |= 1 << bit_offset;
}

void clearBit(int i, uint8_t* bitset){
    int byte_index = i/8;
    int bit_offset = i%8;
    bitset[byte_index] &= ~(1 << bit_offset);
}

int readBit(int i, uint8_t* bitset){
    int byte_index = i/8;
    int bit_offset = i%8;
    return bitset[byte_index] & (1 << bit_offset);
}

/**
 * @brief Get the Superblock object
 * 
 * @param disk_size in bytes
 * @return struct superblock 
 */
struct superblock get_superblock(int disk_size){
    // Calculate available space (excluding superblock)
    int available_space = disk_size - sizeof(struct superblock);
    
    // Calculate inode space and count
    int inode_space = available_space * I_NODE_RATIO;
    int inode_count = inode_space / sizeof(struct pseudo_inode);
    
    // Calculate bitmap sizes (in bytes)
    int inode_bitmap_bytes = (inode_count + 7) / 8;  // +7 for ceiling division
    
    // Calculate data space and cluster count
    int data_space = available_space - inode_space - inode_bitmap_bytes;
    int cluster_count = ((data_space * 8) / (CLUSTER_SIZE + 1))/8;
    int cluster_bitmap_bytes = (cluster_count + 7) / 8;
    
    // Recalculate data space
    // int actual_data_space = data_space - cluster_bitmap_bytes;
    
    // Calculate addresses
    int bitmapi_start_address = sizeof(struct superblock);
    int bitmap_start_address = bitmapi_start_address + inode_bitmap_bytes;
    int inode_start_address = bitmap_start_address + cluster_bitmap_bytes;
    int data_start_address = inode_start_address + inode_space;

    struct superblock sup = {
        .disk_size = disk_size,
        .cluster_size = CLUSTER_SIZE,
        .cluster_count = cluster_count,
        .bitmapi_start_address = bitmapi_start_address,
        .bitmap_start_address = bitmap_start_address,
        .inode_start_address = inode_start_address,
        .data_start_address = data_start_address,
    };

    return sup;
}


int format(char* filename, int size){
    // TODO: Implement format function
    FILE* fptr = fopen(filename,"w");
    if (fptr == NULL){
        return 1;
    }
    printf("opened file");
    struct superblock sb = get_superblock(size);
    uint8_t *memptr = calloc(1,sizeof(char)*size);
    memcpy(memptr, &sb, sizeof(struct superblock));

    int wrote_bytes = fwrite(memptr, sizeof(uint8_t), size, fptr);
    printf("bytes written = %d",wrote_bytes);
    
    free(memptr);
    return 0;
}

int main(){
    format("hejd.ula",1000000);
}