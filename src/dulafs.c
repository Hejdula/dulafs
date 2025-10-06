#include <stdio.h>
#include "dulafs.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define I_NODE_RATIO 0.1 
#define CLUSTER_SIZE 1024 //bytes
#define ROOT_NODE 0


const int ID_ITEM_FREE = 0;

// Global system state
struct SystemState g_system_state = {
    .current_dir = "/",      // Safe - copies string to buffer
    .file_name = "",         // Initialize as empty string
    .file_ptr = NULL,
    .sb = {0}
};


void setBit(int i, int bitmap_offset, FILE* fptr){
    int byte_index = i/8;
    int bit_offset = i%8;
    int byte_position = byte_index + bitmap_offset;
    fseek(fptr, byte_position, SEEK_SET);
    uint8_t byte = fgetc(fptr); 
    byte |= 1 << bit_offset; 
    fseek(fptr, byte_position, SEEK_SET);
    fputc(byte, fptr);
    fflush(fptr);
}

void clearBit(int i, int bitmap_offset, FILE* fptr){
    int byte_index = i/8;
    int bit_offset = i%8;
    fseek(fptr, byte_index + bitmap_offset, SEEK_SET);
    uint8_t byte = fgetc(fptr); 
    byte &= ~(1 << bit_offset);
    fseek(fptr, byte_index + bitmap_offset, SEEK_SET);
    fputc(byte, fptr);
    fflush(fptr);
}

int readBit(int i, int bitmap_offset, FILE* fptr){
    int byte_index = i/8;
    int bit_offset = i%8;
    fseek(fptr, byte_index + bitmap_offset, SEEK_SET);
    uint8_t byte = fgetc(fptr);
    return (byte >> bit_offset) & 1;
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
    int inode_count = inode_space / sizeof(struct inode);
    
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
        .signature = {'H','E','J','D','U','L','A','\0'},
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

struct inode get_inode_struct(bool is_file){
        struct inode inode;
        memset(&inode, 0, sizeof(struct inode));
        inode.node_id = 0;
        inode.is_file = is_file;
        inode.references = 1;
        return inode;
}

int get_empty_index(int bitmap_offset, FILE* fptr){
    int byte_index = 0;
    int bit_offset = 0;
    // search for first unset bit
    fseek(fptr, bitmap_offset, SEEK_SET);
    uint8_t byte;
    while ((byte = fgetc(fptr)) == 255) byte_index++;
    while (byte >> bit_offset & 1) bit_offset++; 

    return byte_index * 8 + bit_offset;
}

int create_dir(){
    struct inode inode = get_inode_struct(false);
    return 0;
}


int format(int size){
    // TODO: Implement format function

    struct superblock sb = get_superblock(size);
    uint8_t *memptr = calloc(1,sizeof(char)*size);
    memcpy(memptr, &sb, sizeof(struct superblock));
    g_system_state.sb = sb;

    fseek(g_system_state.file_ptr, 0, SEEK_SET);
    int bytes_written = fwrite(memptr, sizeof(uint8_t), size, g_system_state.file_ptr);
    printf("bytes written = %d\n",bytes_written);
    
    free(memptr);

    printf("\nSuperblock info:\n");
    printf("Signature: '%.8s'\n", sb.signature);
    printf("Disk size: %d bytes\n", sb.disk_size);
    printf("Cluster size: %d bytes\n", sb.cluster_size);
    printf("Cluster count: %d\n", sb.cluster_count);
    printf("Inode bitmap start address: %d\n", sb.bitmapi_start_address);
    printf("Cluster bitmap start address: %d\n", sb.bitmap_start_address);
    printf("Inode start address: %d\n", sb.inode_start_address);
    printf("Data start address: %d\n", sb.data_start_address);

    return 0;
}

int test() {
    
    printf("=== Test Complete ===\n");
    return 0;
}