#ifndef DULAFS_H
#define DULAFS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define MAX_DIR_PATH 1024
#define DIRECT_CLUSTER_COUNT 5
#define ROOT_NODE 0
#define DIR_NAME_SIZE 12
#define I_NODE_RATIO 0.02 
#define CLUSTER_SIZE 1024

extern const int ID_ITEM_FREE;

struct superblock {
//   char signature[9];             // login autora FS
//   char volume_descriptor[251];   // popis vygenerovaného FS
  char signature[8];
  int disk_size;             // celkova velikost VFS
  int cluster_size;          // velikost clusteru
  int cluster_count;         // pocet clusteru
  int inode_count;
  int bitmapi_start_address; // adresa pocatku bitmapy i-uzlů
  int bitmap_start_address;  // adresa pocatku bitmapy datových bloků
  int inode_start_address;   // adresa pocatku  i-uzlů
  int data_start_address;    // adresa pocatku datovych bloku
};

// System state structure
struct SystemState {
  char working_dir[1024];
  FILE* file_ptr;
  int curr_node_id;
  struct superblock sb;
};

struct inode {
  int id;      // ID i-uzlu, pokud ID = ID_ITEM_FREE, je polozka volna
  bool is_file;    // soubor, nebo adresar
  int8_t references;    // počet odkazů na i-uzel, používá se pro hardlinky
  int file_size;    // velikost souboru v bytech
  int direct[DIRECT_CLUSTER_COUNT];      // 1. přímý odkaz na datové bloky
  int indirect1;    // 1. nepřímý odkaz (odkaz - datové bloky)
  int indirect2;    // 2. nepřímý odkaz (odkaz - odkaz - datové bloky)
};

struct directory_item {
  int inode;      // inode odpovídající souboru
  char item_name[DIR_NAME_SIZE]; // 8+3 + /0 C/C++ ukoncovaci string znak
};

extern struct SystemState g_system_state;

// Function declarations
void setBit(int i, int bitmap_offset);
void clearBit(int i, int bitmap_offset);
int readBit(int i, int bitmap_offset);

struct superblock get_superblock(int disk_size);
struct inode get_inode_struct(bool is_file);
int get_empty_index(int bitmap_offset);
uint8_t* get_node_data(struct inode* inode);
int* get_node_clusters(struct inode* inode);
struct inode get_inode(int node_id);
int contains_file(struct inode* inode, char* file_name);


void delete_inode(struct inode *inode);
int create_dir_node(int up_ref);
void write_inode(struct inode *inode);
int add_record_to_dir(struct directory_item record, struct inode* inode);
int assign_empty_inode();
int assign_empty_cluster();
int* assign_node_clusters(struct inode* inode);
int format(int size);
char* inode_to_path(int inode_id);
int path_to_inode(char* path);
int delete_item(struct inode* inode, char* item_name);
int test();

#endif