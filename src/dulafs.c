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


void set_bit(int i, int bitmap_offset){
    int byte_index = i/8;
    int bit_offset = i%8;
    int byte_position = byte_index + bitmap_offset;
    fseek(g_system_state.file_ptr, byte_position, SEEK_SET);
    uint8_t byte = fgetc(g_system_state.file_ptr); 
    byte |= 1 << bit_offset; 
    fseek(g_system_state.file_ptr, byte_position, SEEK_SET);
    fputc(byte, g_system_state.file_ptr);
    fflush(g_system_state.file_ptr);
}

void clear_bit(int i, int bitmap_offset){
    int byte_index = i/8;
    int bit_offset = i%8;
    fseek(g_system_state.file_ptr, byte_index + bitmap_offset, SEEK_SET);
    uint8_t byte = fgetc(g_system_state.file_ptr); 
    byte &= ~(1 << bit_offset);
    fseek(g_system_state.file_ptr, byte_index + bitmap_offset, SEEK_SET);
    fputc(byte, g_system_state.file_ptr);
    fflush(g_system_state.file_ptr);
}

int read_bit(int i, int bitmap_offset){
    int byte_index = i/8;
    int bit_offset = i%8;
    fseek(g_system_state.file_ptr, byte_index + bitmap_offset, SEEK_SET);
    uint8_t byte = fgetc(g_system_state.file_ptr);
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

int get_empty_index(int bitmap_offset){
    int byte_index = 0;
    int bit_offset = 0;
    // search for first unset bit
    fseek(g_system_state.file_ptr, bitmap_offset, SEEK_SET);
    uint8_t byte;
    while ((byte = fgetc(g_system_state.file_ptr)) == 255) byte_index++;
    while (byte >> bit_offset & 1) bit_offset++;
    return byte_index * 8 + bit_offset;
}

int get_empty_cluster(){
    int cluster_id = get_empty_index(g_system_state.sb.bitmap_start_address);
    set_bit(cluster_id, g_system_state.sb.bitmap_start_address);
    return cluster_id;
}

int create_dir_node(){
    struct inode inode;
    memset(&inode, 0, sizeof(struct inode));
    inode.is_file = false;
    inode.id = get_empty_index(g_system_state.sb.inode_start_address);
    inode.direct[0] = get_empty_cluster();

    return inode.id;
}

struct inode get_inode(int node_id){
    struct inode inode;
    fseek(g_system_state.file_ptr, node_id * sizeof(struct inode), SEEK_SET);
    fread(&inode,sizeof(struct inode),1,g_system_state.file_ptr);
    return inode;
}

int* get_node_clusters(struct inode* inode){
    int cluster_count = (inode->file_size+CLUSTER_SIZE-1)/CLUSTER_SIZE;
    int* carr = malloc(cluster_count*sizeof(int));
    for (int i = 0; i < cluster_count && i < DIRECT_CLUSTER_COUNT; i++){
        if (inode->direct[i]) carr[i] = inode->direct[i];
        else break;
    }
    return carr;
}

uint8_t* get_node_data(struct inode* inode){
    int* cluster_arr = get_node_clusters(inode); 
    int cluster_count = (inode->file_size+CLUSTER_SIZE-1)/CLUSTER_SIZE;
    uint8_t* data = malloc(inode->file_size);

    int bytes_to_read = CLUSTER_SIZE;
    for(int i = 0; i < cluster_count; i++){    
        if((i + 1) * CLUSTER_SIZE > inode->file_size){
            bytes_to_read = inode->file_size - i * CLUSTER_SIZE;
        }
        fseek(g_system_state.file_ptr, cluster_arr[i] * CLUSTER_SIZE + g_system_state.sb.data_start_address, SEEK_SET);
        fread(data + CLUSTER_SIZE * i, bytes_to_read, 1, g_system_state.file_ptr);
    }
    free(cluster_arr);
    return data;
};

// int read_dir_contents(struct directory_item* dptr,int node_id){
//     struct inode inode = get_inode(node_id);
//     struct directory_item* data = (struct directory_item*) get_node_data(&inode);
//     return 0;
// }

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