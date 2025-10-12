#include <stdio.h>
#include "dulafs.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define I_NODE_RATIO 0.1 
#define CLUSTER_SIZE 1024


const int ID_ITEM_FREE = 0;

// Global system state
struct SystemState g_system_state = {
    .working_dir = "/",      // Safe - copies string to buffer
    .file_ptr = NULL,
    .curr_node_id = ROOT_NODE,
    .sb = {0}
};


void set_bit(int i, int bitmap_offset){
    printf("setting bit:%d\n", i);
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

void write_inode(struct inode* inode){
    long offset = g_system_state.sb.inode_start_address + inode->id * sizeof(struct inode);
    fseek(g_system_state.file_ptr, offset, SEEK_SET);
    fwrite(inode, sizeof(struct inode), 1, g_system_state.file_ptr);
    fflush(g_system_state.file_ptr);
}


int get_empty_index(int bitmap_offset){
    int byte_index = 0;
    int bit_offset = 0;
    // search for first unset bit

    fseek(g_system_state.file_ptr, bitmap_offset, SEEK_SET);
    uint8_t byte;
    while ((byte = fgetc(g_system_state.file_ptr)) == 255) byte_index++;
    while (byte >> bit_offset & 1) bit_offset++; 
    printf("final offset in get_empty_index: %d\n", byte_index * 8 + bit_offset);
    return byte_index * 8 + bit_offset;
}

int assign_empty_inode(){
    int node_id = get_empty_index(g_system_state.sb.bitmapi_start_address);
    set_bit(node_id, g_system_state.sb.bitmapi_start_address);
    return node_id;
}

int assign_empty_cluster(){
    int cluster_id = get_empty_index(g_system_state.sb.bitmap_start_address);
    set_bit(cluster_id, g_system_state.sb.bitmap_start_address);
    return cluster_id;
}

struct inode get_inode(int node_id){
    struct inode inode;
    fseek(g_system_state.file_ptr, g_system_state.sb.inode_start_address + node_id * sizeof(struct inode), SEEK_SET);
    fread(&inode, sizeof(struct inode), 1, g_system_state.file_ptr);
    return inode;
}

char* inode_to_path(int inode_id){
    struct inode curr_inode = get_inode(inode_id);
    int prev_inode_id = -1;
    char* path = malloc(MAX_DIR_PATH);
    if (!path) return NULL;
    path[0] = '\0';
    while (prev_inode_id != ROOT_NODE) {
        struct directory_item* dir_content = (struct directory_item*) get_node_data(&curr_inode);
        if (prev_inode_id != -1) {
            int record_found = 0;
            int record_count = curr_inode.file_size / sizeof(struct directory_item);
            for (int i = 0; i < record_count; i++) {
                if (dir_content[i].inode == prev_inode_id) {
                    // Prepend the name to the path
                    char temp[MAX_DIR_PATH];
                    snprintf(temp, sizeof(temp), "/%s%s", dir_content[i].item_name, path);
                    strlcpy(path, temp, MAX_DIR_PATH);
                    record_found = 1;
                    break;
                }
            }
            if (!record_found) {
                // Could not find name in parent
                free(path);
                free(dir_content);
                return NULL;
            }
        }
        prev_inode_id = curr_inode.id;
        curr_inode = get_inode(dir_content[0].inode);

        free(dir_content);
    }
    // If path is empty, we are at root
    if (path[0] == '\0') {
        strlcpy(path, "/", MAX_DIR_PATH);
    }
    return path;
}

int path_to_inode(char* path){
    if (strlen(path) >= MAX_DIR_PATH){
        fprintf(stderr, "Path too long\n");
        return -1;
    }

    int curr_node_id;
    if (path[0] == '/') curr_node_id = ROOT_NODE;
    else curr_node_id = g_system_state.curr_node_id ;


    char path_copy[MAX_DIR_PATH];
    strncpy(path_copy, path, MAX_DIR_PATH);
    // process tokens by one of the path
    for (char* token = strtok(path_copy, "/"); token != NULL; token = strtok(NULL, "/")){
        // if(!strcmp(token, ".")){ continue; }
        struct inode inode = get_inode(curr_node_id);
        if (inode.is_file){
            fprintf(stderr, "file: \"%s\" can not be traversed like a directory\n", token);
            return -1;
        }
        struct directory_item* node_data = (struct directory_item*) get_node_data(&inode);
        int record_count = inode.file_size / sizeof(struct directory_item);
        int node_found = 0;
        for (int i = 0; i < record_count; i++){
            if (!strcmp(token, node_data[i].item_name)){
                curr_node_id = node_data[i].inode;
                node_found = 1;
                break;
            }
        }
    
        free(node_data);

        if (!node_found){
            fprintf(stderr, "path does not exist: %s\n", path);
            return -1;
        }
    }
    
    return curr_node_id;
}

int* get_node_clusters(struct inode* inode){
    int cluster_count = (inode->file_size + CLUSTER_SIZE - 1) / CLUSTER_SIZE;
    if(!cluster_count){
        return NULL;
    }
    int* carr = malloc(cluster_count * sizeof(int));
    if (!carr) return NULL;
    int i;
    for (i = 0; i < cluster_count && i < DIRECT_CLUSTER_COUNT; i++){
        carr[i] = inode->direct[i];
    }

    if (!inode->indirect1) return carr;

    int max_1st_indirect = CLUSTER_SIZE / sizeof(int);
    
    // read 1st level indirect
    int* indirect_arr = malloc(CLUSTER_SIZE);
    if (!indirect_arr) { free(carr); return NULL; }

    int offset = g_system_state.sb.data_start_address + inode->indirect1 * CLUSTER_SIZE;
    fseek(g_system_state.file_ptr, offset, SEEK_SET);
    fread(indirect_arr, CLUSTER_SIZE, 1, g_system_state.file_ptr);
    
    // iterate through the clusters
    for (i = i; i < cluster_count && i < (max_1st_indirect + DIRECT_CLUSTER_COUNT); i++){
        carr[i] = indirect_arr[i - DIRECT_CLUSTER_COUNT];
    }


    if (!inode->indirect2) {
        free(indirect_arr);
        return carr;
    }
    int max_2nd_indirect = max_1st_indirect * max_1st_indirect;

    // read first page of 2nd level indirect
    int* indirect_clusters = malloc(CLUSTER_SIZE);
    if (!indirect_clusters) { free(carr); free(indirect_arr); return NULL; }

    offset = g_system_state.sb.data_start_address + inode->indirect2 * CLUSTER_SIZE;
    fseek(g_system_state.file_ptr, offset, SEEK_SET);
    fread(indirect_clusters, CLUSTER_SIZE, 1, g_system_state.file_ptr);

    int max_total = DIRECT_CLUSTER_COUNT + max_1st_indirect + max_2nd_indirect;
    
    // iterate through pages of 1st level indirect
    for (int indirect_index = 0; i < cluster_count && i < cluster_count && i < max_total; indirect_index++){
        offset = g_system_state.sb.data_start_address + indirect_clusters[indirect_index] * CLUSTER_SIZE;
        fseek(g_system_state.file_ptr, offset, SEEK_SET);
        fread(indirect_arr, CLUSTER_SIZE, 1, g_system_state.file_ptr);

        // iterate through the clusters
        for (int j = 0; j < max_1st_indirect && i < cluster_count; j++, i++){
            carr[i] = indirect_arr[j];
        }
    }

    free(indirect_arr);
    free(indirect_clusters);
    return carr;
}

uint8_t* get_node_data(struct inode* inode){
    if (!inode->file_size) return NULL;
    int* cluster_arr = get_node_clusters(inode);
    int cluster_count = (inode->file_size+CLUSTER_SIZE-1)/CLUSTER_SIZE;
    uint8_t* data = malloc(inode->file_size);

    for (int i = 0; i < cluster_count; i++){    
        int bytes_to_read = CLUSTER_SIZE;
        if ((i + 1) * CLUSTER_SIZE > inode->file_size){
            bytes_to_read = inode->file_size - i * CLUSTER_SIZE;
        }
        fseek(g_system_state.file_ptr, cluster_arr[i] * CLUSTER_SIZE + g_system_state.sb.data_start_address, SEEK_SET);
        fread(data + CLUSTER_SIZE * i, bytes_to_read, 1, g_system_state.file_ptr);
    }
    free(cluster_arr);
    return data;
};

int delete_item(int inode_id, char* item_name){
    struct inode inode = get_inode(inode_id);
    if (inode.is_file){
        fprintf(stderr, "not a directory");
        return EXIT_FAILURE;
    }
    struct directory_item* dir_content = (struct directory_item*) get_node_data(&inode);
    int record_count = inode.file_size / sizeof(struct directory_item);
    int item_found = 0;
    // loop through directory items to find the one to delete
    for (int i = 0; i < record_count; i++){
        if (!strcmp(dir_content[i].item_name, item_name)){
            item_found = 1;
            struct inode inode_to_delete = get_inode(dir_content[i].inode);
            
            // remove item from directory
            struct directory_item empty_item = {0};
            int offset = g_system_state.sb.data_start_address + inode.direct[0] * CLUSTER_SIZE + i * sizeof(struct directory_item);
            fseek(g_system_state.file_ptr, offset, SEEK_SET);
            fwrite(&empty_item, sizeof(struct directory_item), 1, g_system_state.file_ptr);

            // set the inode as free in bitmap
            clear_bit(inode_to_delete.id, g_system_state.sb.bitmapi_start_address);

            // free the inode clusters
            int* clusters = get_node_clusters(&inode_to_delete);
            if (clusters != NULL){
                int cluster_count = (inode_to_delete.file_size+CLUSTER_SIZE-1)/CLUSTER_SIZE;
                for (int j = 0; j < cluster_count; j++){
                    clear_bit(clusters[j], g_system_state.sb.bitmap_start_address);
                }
            }
            free(clusters);

            break;
        }
    }
    free(dir_content);
    if(!item_found){
        fprintf(stderr, "Could not find %s\n", item_name);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int add_record_to_dir(struct directory_item record, struct inode* inode){ 
    struct directory_item* node_data = (struct directory_item*) get_node_data(inode);

    if(inode->file_size == 0){
        inode->direct[0] = assign_empty_cluster();
    }

    int record_count = inode->file_size / sizeof(struct directory_item);
    int final_offset;
    int empty_space_found = 0;

    struct directory_item current;
    for (int i = 0; i < record_count; i++){
        current = node_data[i];
        if (!current.item_name[0]){
            empty_space_found = 1;
            int offset_in_cluster = (i * sizeof(struct directory_item)) % CLUSTER_SIZE;
            int item_addr = inode->direct[0] * CLUSTER_SIZE + offset_in_cluster;
            final_offset = g_system_state.sb.data_start_address + item_addr;
            break;
        }
    }

    if(!empty_space_found){
        final_offset = g_system_state.sb.data_start_address + inode->direct[0] * CLUSTER_SIZE + inode->file_size;
    }

    if(!final_offset){
        free(node_data);
        return EXIT_FAILURE;
    }

    printf("final offset in addrecord to dir: %d\n", final_offset);
    fseek(g_system_state.file_ptr, final_offset, SEEK_SET);
    fwrite(&record, sizeof(struct directory_item), 1, g_system_state.file_ptr);

    inode->file_size += sizeof(struct directory_item);
    write_inode(inode);

    free(node_data);
    return EXIT_SUCCESS;
}

int create_dir_node(int up_ref_id){
    struct inode inode;
    memset(&inode, 0, sizeof(struct inode));
    inode.is_file = false;
    inode.id = assign_empty_inode();

    struct directory_item up_ref = {up_ref_id,"..\0\0\0\0\0\0\0\0\0\0"};
    struct directory_item self_ref = {inode.id,".\0\0\0\0\0\0\0\0\0\0\0"}; 

    add_record_to_dir(up_ref, &inode);
    add_record_to_dir(self_ref, &inode);

    write_inode(&inode);

    return inode.id;
}

int format(int size){

    struct superblock sb = get_superblock(size);
    uint8_t *memptr = calloc(1,sizeof(char)*size);
    memcpy(memptr, &sb, sizeof(struct superblock));
    g_system_state.sb = sb;

    fseek(g_system_state.file_ptr, 0, SEEK_SET);
    int bytes_written = fwrite(memptr, sizeof(uint8_t), size, g_system_state.file_ptr);
    printf("bytes written = %d\n",bytes_written); 
    free(memptr);

    create_dir_node(ROOT_NODE);

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