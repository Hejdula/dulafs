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

    int true_disk_size = disk_size - sizeof(struct superblock);
    int inode_disk_size = true_disk_size * I_NODE_RATIO;
    int inode_bitset_bytes = inode_disk_size / sizeof(struct pseudo_inode)/8;
    int cluster_bitset_bytes = (true_disk_size - inode_disk_size - inode_bitset_bytes)/CLUSTER_SIZE/8;
    int cluster_disk_size = true_disk_size - (inode_disk_size + inode_bitset_bytes + cluster_bitset_bytes);
    int cluster_count = cluster_disk_size;

    struct superblock sup = {
        .disk_size = disk_size,
        .cluster_size = CLUSTER_SIZE,
        .cluster_count = 0,
        .bitmapi_start_address = 0,
        .bitmap_start_address = 0,
        .inode_start_address = 0,
        .data_start_address = 0,
    };
    return sup;
}


int format(char* filename, int size){

}