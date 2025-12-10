#include <stdio.h>
#include "dulafs.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const long long int MAX_FILE_SIZE = (DIRECT_CLUSTER_COUNT + (CLUSTER_SIZE / sizeof(int) + 1) * (CLUSTER_SIZE / sizeof(int))) * CLUSTER_SIZE;

// Global system state
struct SystemState g_system_state = {
    .working_dir = "/",      // Safe - copies string to buffer
    .file_ptr = NULL,
    .curr_node_id = ROOT_NODE,
    .sb = {0}
};

/**
 * @brief Returns the string representation of an error code.
 * 
 * @param code The error code to translate.
 * @return const char* The error message string, or NULL if success.
 */
const char* get_error_message(ErrorCode code) {
    switch (code) {
        case ERR_SUCCESS: return NULL;
        case ERR_NO_SOURCE: return "Source file not found";
        case ERR_INVALID_SIZE: return "Invalid size argument";
        case ERR_PATH_NOT_EXIST: return "Path not found";
        case ERR_NOT_A_DIRECTORY: return "Not a directory";
        case ERR_NOT_A_FILE: return "Not a file";
        case ERR_FILE_EXISTS: return "File or directory with the same name already exists";
        case ERR_INODE_FULL: return "No free inodes available";
        case ERR_CLUSTER_FULL: return "No free clusters available";
        case ERR_DIR_NOT_EMPTY: return "Target directory is not empty";
        case ERR_FILE_NOT_FOUND: return "File not found";
        case ERR_MEMORY_ALLOCATION: return "Memory allocation failed";
        case ERR_CANNOT_TRAVERSE: return "Cannot traverse file as directory";
        case ERR_CANNOT_REMOVE_DOT: return "Cannot remove '.' or '..' from directory";
        case ERR_EXTERNAL_FILE_NOT_FOUND: return "External file not found";
        case ERR_CANNOT_HARDLINK_DIR: return "Cannot hard link a directory";
        case ERR_INVALID_ARGC: return "Invalid number of arguments"; 
        case ERR_FILE_TOO_LARGE: return "File too large";
        default: return "Unknown error";
    }
}

/**
 * @brief Set i-th bit in a given bitmap
 * 
 * @param i index of bit, starting from 0
 * @param bitmap_offset bitmap to operate on
 */
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

/**
 * @brief Clear i-th bit in a given bitmap.
 * 
 * @param i Index of bit, starting from 0.
 * @param bitmap_offset Byte offset of the bitmap in the file.
 */
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

/**
 * @brief Read the value of the i-th bit in a given bitmap.
 * 
 * @param i Index of bit, starting from 0.
 * @param bitmap_offset Byte offset of the bitmap in the file.
 * @return int The value of the bit (0 or 1).
 */
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
 * @return struct superblock The initialized superblock structure.
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
        .inode_count = inode_count,
        .bitmapi_start_address = bitmapi_start_address,
        .bitmap_start_address = bitmap_start_address,
        .inode_start_address = inode_start_address,
        .data_start_address = data_start_address,
    };    


    return sup;
}

/**
 * @brief Write an inode structure to disk.
 * 
 * @param inode Pointer to the inode structure to write.
 */
void write_inode(struct inode* inode){
    long offset = g_system_state.sb.inode_start_address + inode->id * sizeof(struct inode);
    fseek(g_system_state.file_ptr, offset, SEEK_SET);
    fwrite(inode, sizeof(struct inode), 1, g_system_state.file_ptr);
    fflush(g_system_state.file_ptr);
}

/**
 * @brief Find the index of the first unset (0) bit in a bitmap.
 * 
 * @param bitmap_offset Byte offset of the bitmap in the file.
 * @return int The index of the first empty bit.
 */
int get_empty_index(int bitmap_offset){
    int byte_index = 0;
    int bit_offset = 0;
    // search for first unset bit

    fseek(g_system_state.file_ptr, bitmap_offset, SEEK_SET);
    uint8_t byte;
    while ((byte = fgetc(g_system_state.file_ptr)) == 255) byte_index++;
    while (byte >> bit_offset & 1) bit_offset++; 
    // printf("final offset in get_empty_index: %d\n", byte_index * 8 + bit_offset);
    return byte_index * 8 + bit_offset;
}

/**
 * @brief Calculate the number of unused inodes remaining.
 * 
 * @return int Number of free inodes.
 */
int unused_inodes_left(){
    return g_system_state.sb.inode_count - count_ones(g_system_state.sb.bitmapi_start_address, g_system_state.sb.inode_count);
};

/**
 * @brief Find a free inode, mark it as used, and return its ID.
 * 
 * @return int The ID of the assigned inode, or -1 if full.
 */
int assign_empty_inode(){
    int node_id = get_empty_index(g_system_state.sb.bitmapi_start_address);
    if (node_id >= g_system_state.sb.inode_count) return -1;
    set_bit(node_id, g_system_state.sb.bitmapi_start_address);
    return node_id;
}

/**
 * @brief Find a free cluster, mark it as used, and return its ID.
 * 
 * @return int The ID of the assigned cluster, or -1 if full.
 */
int assign_empty_cluster(){
    int cluster_id = get_empty_index(g_system_state.sb.bitmap_start_address);
    if (cluster_id >= g_system_state.sb.cluster_count) return -1;
    set_bit(cluster_id, g_system_state.sb.bitmap_start_address);
    return cluster_id;
}

/**
 * @brief Read an inode from disk by its ID.
 * 
 * @param node_id The ID of the inode to read.
 * @return struct inode The inode structure read from disk.
 */
struct inode get_inode(int node_id){
    struct inode inode;
    fseek(g_system_state.file_ptr, g_system_state.sb.inode_start_address + node_id * sizeof(struct inode), SEEK_SET);
    fread(&inode, sizeof(struct inode), 1, g_system_state.file_ptr);
    return inode;
}

/**
 * @brief Check if a directory contains a file with the given name.
 * 
 * @param inode Pointer to the directory inode.
 * @param file_name Name of the file to search for.
 * @return int 1 if found, 0 otherwise.
 */
int contains_file(struct inode* inode, char* file_name){
    if (inode->is_file) {
        return 0;
    }
    
    struct directory_item* dir_content = get_directory_items(inode);
    if (!dir_content) return 0;
    
    int record_count = inode->file_size / sizeof(struct directory_item);
    int found = 0;
    
    for (int i = 0; i < record_count; i++){
        if (!strcmp(dir_content[i].item_name, file_name)){
            found = 1;
            break;
        }
    }
    
    free(dir_content);
    return found;
}

/**
 * @brief Reconstruct the full path string for a given inode ID.
 * 
 * @param inode_id The ID of the inode.
 * @return char* The full path string (must be freed by caller), or NULL on error.
 */
char* inode_to_path(int inode_id){
    struct inode curr_inode = get_inode(inode_id);
    int prev_inode_id = -1;
    char* path = malloc(MAX_DIR_PATH);
    if (!path) return NULL;
    path[0] = '\0';
    while (prev_inode_id != ROOT_NODE) {
        struct directory_item* dir_content = get_directory_items(&curr_inode);
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

/**
 * @brief Extract and return pointer to the final token/name from a path.
 * 
 * @param path The full path string.
 * @return char* Pointer to the last component of the path.
 */
char* get_final_token(char* path) {
    if (!path) return NULL;
    
    char* last_slash = strrchr(path, '/');
    return last_slash ? last_slash + 1 : path;
}

/**
 * @brief Get the dir id object returns id of directory which contains the path target,
 * also set the target_name to point into path to the target name
 * 
 * @param path The full path to the target.
 * @param target_name Output pointer to the start of the target name within the path string.
 * @return int The inode ID of the parent directory, or negative error code.
 */
int get_dir_id(char* path, char** target_name){
    size_t length = strlen(path);
    // invalid path if ends with '/'
    if (length > 1 && path[length] == '/'){ return -ERR_PATH_NOT_EXIST; }

    char* path_copy = malloc(length + 1);
    if(!path_copy){ return -ERR_MEMORY_ALLOCATION; }
    strncpy(path_copy, path, length + 1);

    char* last_slash = strrchr(path_copy,'/');
    if (last_slash){
        *last_slash = '\0';
        *target_name = last_slash - path_copy + path + 1;
    } else {
        free(path_copy);
        *target_name = path;
        return g_system_state.curr_node_id;
    }
    int retval = path_to_inode(path_copy);
    free(path_copy);
    return retval;
}

/**
 * @brief Resolve a path string to an inode ID.
 * 
 * @param path The path to resolve.
 * @return int The inode ID, or negative error code.
 */
int path_to_inode(char* path){
    // invalid path if ends with '/'
    size_t length = strlen(path);
    if (length > 1 && path[length] == '/'){ return -ERR_PATH_NOT_EXIST; }

    int curr_node_id;
    if (path[0] == '/') curr_node_id = ROOT_NODE;
    else curr_node_id = g_system_state.curr_node_id ;


    char* path_copy = strdup(path);
    
    // process tokens by one of the path
    for (char* token = strtok(path_copy, "/"); token != NULL; token = strtok(NULL, "/")){
        // if(!strcmp(token, ".")){ continue; }
        struct inode inode = get_inode(curr_node_id);
        if (inode.is_file){
            free(path_copy);
            return -ERR_CANNOT_TRAVERSE;
        }
        struct directory_item* node_data = get_directory_items(&inode);
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
            free(path_copy);
            return -ERR_PATH_NOT_EXIST;
        }
    }
    
    free(path_copy);
    return curr_node_id;
}
    
/**
 * @brief "Allocate" clusters for an inode based on its size, handling direct and indirect blocks.
 * Sets the bits of relevant clusters to full in the bitmap.
 * @param inode Pointer to the inode to assign clusters to.
 * @return int* Array of assigned cluster IDs (must be freed), or NULL on failure.
 */
int* assign_node_clusters(struct inode* inode){
    int cluster_count = (inode->file_size + CLUSTER_SIZE - 1) / CLUSTER_SIZE;
    if(!cluster_count){
        return NULL;
    }
    
    int* carr = malloc(cluster_count * sizeof(int));
    if (!carr) return NULL;
    
    int i;
    // Assign direct clusters
    for (i = 0; i < cluster_count && i < DIRECT_CLUSTER_COUNT; i++){
        carr[i] = assign_empty_cluster();
        inode->direct[i] = carr[i];
    }
    
    if (i >= cluster_count) {
        write_inode(inode);
        return carr;
    }
    
    int max_1st_indirect = CLUSTER_SIZE / sizeof(int);
    
    // Need 1st level indirect
    int* indirect_arr = calloc(max_1st_indirect, sizeof(int));
    if (!indirect_arr) { free(carr); return NULL; }
    
    // Assign indirect1 cluster if not already assigned
    if (!inode->indirect1) {
        inode->indirect1 = assign_empty_cluster();
    }
    
    // Assign clusters through 1st level indirect
    for (int direct_index = 0; i < cluster_count && direct_index < max_1st_indirect; direct_index++, i++){
        carr[i] = assign_empty_cluster();
        indirect_arr[direct_index] = carr[i];
    }
    
    // Write 1st level indirect to disk
    int offset = g_system_state.sb.data_start_address + inode->indirect1 * CLUSTER_SIZE;
    fseek(g_system_state.file_ptr, offset, SEEK_SET);
    fwrite(indirect_arr, CLUSTER_SIZE, 1, g_system_state.file_ptr);
    fflush(g_system_state.file_ptr);
    
    if (i >= cluster_count) {
        free(indirect_arr);
        write_inode(inode);
        return carr;
    }
    
    int max_2nd_indirect = max_1st_indirect * max_1st_indirect;
    
    // Need 2nd level indirect
    int* indirect_clusters = calloc(max_1st_indirect, sizeof(int));
    if (!indirect_clusters) { free(carr); free(indirect_arr); return NULL; }
    
    // Assign indirect2 cluster if not already assigned
    if (!inode->indirect2) {
        inode->indirect2 = assign_empty_cluster();
    }
    
    int max_total = DIRECT_CLUSTER_COUNT + max_1st_indirect + max_2nd_indirect;
    
    // Iterate through pages of 1st level indirect
    for (int indirect_index = 0; i < cluster_count && i < max_total; indirect_index++){
        // Assign a cluster for this indirect page if needed
        if (!indirect_clusters[indirect_index]) {
            indirect_clusters[indirect_index] = assign_empty_cluster();
        }
        
        // Clear the indirect array for this page
        memset(indirect_arr, 0, CLUSTER_SIZE);
        
        // Assign clusters through this indirect page
        for (int direct_index = 0; direct_index < max_1st_indirect && i < cluster_count; direct_index++, i++){
            carr[i] = assign_empty_cluster();
            indirect_arr[direct_index] = carr[i];
        }
        
        // Write this indirect page to disk
        offset = g_system_state.sb.data_start_address + indirect_clusters[indirect_index] * CLUSTER_SIZE;
        fseek(g_system_state.file_ptr, offset, SEEK_SET);
        fwrite(indirect_arr, CLUSTER_SIZE, 1, g_system_state.file_ptr);
        fflush(g_system_state.file_ptr);
    }
    
    // Write 2nd level indirect clusters array to disk
    offset = g_system_state.sb.data_start_address + inode->indirect2 * CLUSTER_SIZE;
    fseek(g_system_state.file_ptr, offset, SEEK_SET);
    fwrite(indirect_clusters, CLUSTER_SIZE, 1, g_system_state.file_ptr);
    fflush(g_system_state.file_ptr);
    
    free(indirect_arr);
    free(indirect_clusters);
    write_inode(inode);

    return carr;
}

/**
 * @brief Retrieve the array of cluster IDs used by an inode.
 * 
 * @param inode Pointer to the inode.
 * @return int* Array of cluster IDs (must be freed), or NULL if empty/error.
 */
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

    if (!inode->indirect1){ 
        return carr; 
    }

    int max_1st_indirect = CLUSTER_SIZE / sizeof(int);
    
    // read 1st level indirect
    int* indirect_arr = malloc(CLUSTER_SIZE);
    if (!indirect_arr) { free(carr); return NULL; }

    int offset = g_system_state.sb.data_start_address + inode->indirect1 * CLUSTER_SIZE;
    fseek(g_system_state.file_ptr, offset, SEEK_SET);
    fread(indirect_arr, CLUSTER_SIZE, 1, g_system_state.file_ptr);
    
    // iterate through the clusters
    for (int direct_index = 0; i < cluster_count && direct_index < max_1st_indirect; direct_index++, i++){
        carr[i] = indirect_arr[direct_index];
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
        for (int direct_index = 0; direct_index < max_1st_indirect && i < cluster_count; direct_index++, i++){
            carr[i] = indirect_arr[direct_index];
        }
    }

    
    free(indirect_arr);
    free(indirect_clusters);
    return carr;
}

/**
 * @brief Read all data associated with an inode into a buffer.
 * 
 * @param inode Pointer to the inode.
 * @return uint8_t* Buffer containing the data (must be freed), or NULL if empty.
 */
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

/**
 * @brief Helper to get directory items from a directory inode.
 * 
 * @param dir_inode Pointer to the directory inode.
 * @return struct directory_item* Array of directory items (must be freed).
 */
struct directory_item* get_directory_items(struct inode* dir_inode) {
    return (struct directory_item*)get_node_data(dir_inode);
}

/**
 * @brief Free an inode and all its associated clusters/blocks.
 * Clears all bits of inode clusters and the inode itself
 * @param inode Pointer to the inode to clear.
 */
void clear_inode(struct inode* inode){ 
    // set the inode as free in bitmap
    clear_bit(inode->id, g_system_state.sb.bitmapi_start_address);

    // free the inode clusters
    int* clusters = get_node_clusters(inode);
    if (clusters != NULL){
        int cluster_count = (inode->file_size+CLUSTER_SIZE-1)/CLUSTER_SIZE;
        for (int j = 0; j < cluster_count; j++){
            clear_bit(clusters[j], g_system_state.sb.bitmap_start_address);
        }
    }
    free(clusters);

    if(inode->indirect1) {
        clear_bit(inode->indirect1, g_system_state.sb.bitmap_start_address);
    }

    if(inode->indirect2) {
        int* cluster_ids = malloc(CLUSTER_SIZE);
        int offset = g_system_state.sb.data_start_address + inode->indirect2 * CLUSTER_SIZE;
        fseek(g_system_state.file_ptr, offset, SEEK_SET);
        fread(cluster_ids, CLUSTER_SIZE, 1, g_system_state.file_ptr);
        for (int i = 0; i < CLUSTER_SIZE / sizeof(int); i++){
            if (cluster_ids[i] && cluster_ids[i] < g_system_state.sb.cluster_count){ 
                clear_bit(cluster_ids[i], g_system_state.sb.bitmap_start_address);
            }
        }
        clear_bit(inode->indirect2, g_system_state.sb.bitmap_start_address);
        free(cluster_ids);
    }
}

/**
 * @brief Remove a file or directory entry from a parent directory inode.
 * If there are no more references to the item, it is cleared
 * 
 * @param inode Pointer to the parent directory inode.
 * @param item_name Name of the item to remove.
 * @return int Error code (ERR_SUCCESS on success).
 */
int delete_item(struct inode* inode, char* item_name){
    if (inode->is_file){ return ERR_NOT_A_DIRECTORY; }
    struct directory_item* dir_content = get_directory_items(inode);
    int record_count = inode->file_size / sizeof(struct directory_item);
    int item_found = 0;
    // loop through directory items to find the one to delete
    for (int i = 0; i < record_count; i++){
        if (!strcmp(dir_content[i].item_name, item_name)){
            item_found = 1;
            struct inode inode_to_delete = get_inode(dir_content[i].inode);

            // do not delete dir if not empty
            if (inode_to_delete.file_size != 32 && !inode_to_delete.is_file && inode_to_delete.references == 1) {
                free(dir_content);
                return ERR_DIR_NOT_EMPTY;
            }
            
            // remove item from directory by moving last item to this position
            struct directory_item last_item;
            int last_offset = g_system_state.sb.data_start_address + inode->direct[0] * CLUSTER_SIZE + (record_count - 1) * sizeof(struct directory_item);
            fseek(g_system_state.file_ptr, last_offset, SEEK_SET);
            fread(&last_item, sizeof(struct directory_item), 1, g_system_state.file_ptr);
            
            // Write last item to deleted position
            int deleted_offset = g_system_state.sb.data_start_address + inode->direct[0] * CLUSTER_SIZE + i * sizeof(struct directory_item);
            fseek(g_system_state.file_ptr, deleted_offset, SEEK_SET);
            fwrite(&last_item, sizeof(struct directory_item), 1, g_system_state.file_ptr);
            
            
            inode_to_delete.references -= 1;
            if (inode_to_delete.references <= 0){ clear_inode(&inode_to_delete); } 
            else { write_inode(&inode_to_delete); }
            
            break;
        }
    }    
    free(dir_content);
    
    if(!item_found){
        return ERR_FILE_NOT_FOUND;
    }

    // Decrease directory size
    inode->file_size -= sizeof(struct directory_item);
    write_inode(inode);

    return ERR_SUCCESS;
}

/**
 * @brief Add a new entry to a directory inode.
 * 
 * @param record The directory item structure to add.
 * @param dir_inode Pointer to the target directory inode.
 * @return int EXIT_SUCCESS on success.
 */
int add_record_to_dir(struct directory_item record, struct inode* dir_inode){ 
    struct inode added_inode = get_inode(record.inode);
    added_inode.references += 1;
    write_inode(&added_inode);

    // Always append to the end since its compacted on deletion
    int final_offset = g_system_state.sb.data_start_address + dir_inode->direct[0] * CLUSTER_SIZE + dir_inode->file_size;

    fseek(g_system_state.file_ptr, final_offset, SEEK_SET);
    fwrite(&record, sizeof(struct directory_item), 1, g_system_state.file_ptr);

    dir_inode->file_size += sizeof(struct directory_item);
    write_inode(dir_inode);

    return EXIT_SUCCESS;
}

/**
 * @brief Initialize a directory with '.' and '..' entries.
 * 
 * @param dir_inode Pointer to the directory inode to initialize.
 * @param parent_inode_id Inode ID of the parent directory.
 */
void init_directory(struct inode* dir_inode, int parent_inode_id) {
    struct directory_item entries[2] = {0};
    
    // Parent reference (..)
    entries[0].inode = parent_inode_id;
    strlcpy(entries[0].item_name, "..", sizeof(entries[0].item_name));
    
    // Self reference (.)
    entries[1].inode = dir_inode->id;
    strlcpy(entries[1].item_name, ".", sizeof(entries[1].item_name));
    
    // Write both entries directly to the first cluster
    int offset = g_system_state.sb.data_start_address + dir_inode->direct[0] * CLUSTER_SIZE;
    fseek(g_system_state.file_ptr, offset, SEEK_SET);
    fwrite(entries, sizeof(struct directory_item), 2, g_system_state.file_ptr);
    fflush(g_system_state.file_ptr);
    
    // Update directory size
    dir_inode->file_size = 2 * sizeof(struct directory_item);
    write_inode(dir_inode);
}

/**
 * @brief Creates a new directory inode.
 * 
 * @param up_ref_id Inode ID of the parent directory.
 * @return int The ID of the newly created directory inode.
 */
int create_dir_node(int up_ref_id){
    struct inode inode;
    memset(&inode, 0, sizeof(struct inode));
    inode.is_file = false;
    inode.id = assign_empty_inode();
    inode.direct[0] = assign_empty_cluster();
    write_inode(&inode);

    // Initialize directory with . and .. entries
    init_directory(&inode, up_ref_id);

    return inode.id;
}

/**
 * @brief Format the virtual disk with the filesystem structure.
 * 
 * @param size Size of the disk in bytes.
 * @return int Error code (ERR_SUCCESS on success).
 */
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
    printf("Inode count: %d\n", sb.inode_count);
    printf("Inode bitmap start address: %d\n", sb.bitmapi_start_address);
    printf("Cluster bitmap start address: %d\n", sb.bitmap_start_address);
    printf("Inode start address: %d\n", sb.inode_start_address);
    printf("Data start address: %d\n", sb.data_start_address);

    return ERR_SUCCESS;
}

/**
 * @brief Count the number of set ones in a bitmap region.
 * 
 * @param bitmap_offset Byte offset of the bitmap start.
 * @param size Size of the bitmap in bits.
 * @return int Number of set bits.
 */
int count_ones(int bitmap_offset, int size){
    uint8_t* data = malloc((size + 7)/8);
    fseek(g_system_state.file_ptr, bitmap_offset, SEEK_SET);
    fread(data, (size + 7)/8, 1, g_system_state.file_ptr);
    int byte, bit, count = 0;
    for (int i = 0; i < size; i++) {
        byte = i/8;
        bit = i%8;
        if((data[byte] >> bit) & 1) count ++; 
    } 
    free(data);
    return count;
}

/**
 * @brief Checks if there are enough empty clusters available for a file of given size.
 * 
 * @param file_size Size of the file in bytes.
 * @return int 1 if enough space, 0 otherwise.
 */
int enough_empty_clusters(int file_size){
    int empty_cluster_count = g_system_state.sb.cluster_count - count_ones(g_system_state.sb.bitmap_start_address, g_system_state.sb.cluster_count);
    int data_cluster_count = (file_size + CLUSTER_SIZE - 1) / CLUSTER_SIZE ; 
    int pointers_per_cluster = CLUSTER_SIZE / sizeof(int);
    int indirect_2nd_used = (data_cluster_count > DIRECT_CLUSTER_COUNT + pointers_per_cluster) ? 1 : 0;
    int indirect_cluster_count =  (data_cluster_count - DIRECT_CLUSTER_COUNT) / pointers_per_cluster + indirect_2nd_used;
    int total_clusters_needed = indirect_cluster_count + data_cluster_count;
    return empty_cluster_count >= total_clusters_needed; 
}

/**
 * @brief Counts the total number of directories in the system.
 * 
 * @return int Number of directories.
 */
int count_dirs(){
    int used_inodes = count_ones(g_system_state.sb.bitmapi_start_address, g_system_state.sb.inode_count);
    int dir_count = 0;
    for(int i = 0; i < used_inodes; i++){
        struct inode inode = get_inode(i);
        if(!inode.is_file) dir_count++;
    }
    return dir_count;
}

/**
 * @brief Placeholder for test command.
 * 
 * @return int Error code.
 */
int test() {
    
    printf("=== Test Complete ===\n");
    return ERR_SUCCESS;
}