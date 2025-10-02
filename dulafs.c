#include "dulafs.h"
#include <math.h>
#include <stdint.h>

#define I_NODE_RATIO 0.1 
#define CLUSTER_SIZE 1024 //bytes

const int32_t ID_ITEM_FREE = 0;

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

int main(){

}

/**
 * @brief Get the Superblock object
 * 
 * @param disk_size in bytes
 * @return struct superblock 
 */
struct superblock getSuperblock(int disk_size){
    // Calculate available space (excluding superblock)
    int32_t available_space = disk_size - sizeof(struct superblock);
    
    // Calculate inode space and count
    int32_t inode_space = available_space * I_NODE_RATIO;
    int32_t inode_count = inode_space / sizeof(struct pseudo_inode);
    
    // Calculate bitmap sizes (in bytes)
    int32_t inode_bitmap_bytes = (inode_count + 7) / 8;  // +7 for ceiling division
    
    // Calculate data space and cluster count
    int32_t data_space = available_space - inode_space - inode_bitmap_bytes;
    // Reserve space for cluster bitmap (estimate)
    int32_t estimated_clusters = data_space / CLUSTER_SIZE;
    int32_t cluster_bitmap_bytes = (estimated_clusters + 7) / 8;
    
    // Recalculate actual data space and cluster count
    int32_t actual_data_space = data_space - cluster_bitmap_bytes;
    int32_t cluster_count = actual_data_space / CLUSTER_SIZE;
    
    // Calculate addresses
    int32_t bitmapi_start_address = sizeof(struct superblock);
    int32_t bitmap_start_address = bitmapi_start_address + inode_bitmap_bytes;
    int32_t inode_start_address = bitmap_start_address + cluster_bitmap_bytes;
    int32_t data_start_address = inode_start_address + inode_space;

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
    return 0;
}