#ifndef DULAFS_H
#define DULAFS_H

#include <stdint.h>
#include <stdbool.h>

extern const int ID_ITEM_FREE;

struct superblock {
//   char signature[9];             // login autora FS
//   char volume_descriptor[251];   // popis vygenerovaného FS
  int disk_size;             // celkova velikost VFS
  int cluster_size;          // velikost clusteru
  int cluster_count;         // pocet clusteru
  int bitmapi_start_address; // adresa pocatku bitmapy i-uzlů
  int bitmap_start_address;  // adresa pocatku bitmapy datových bloků
  int inode_start_address;   // adresa pocatku  i-uzlů
  int data_start_address;    // adresa pocatku datovych bloku
};

struct pseudo_inode {
  int node_id;      // ID i-uzlu, pokud ID = ID_ITEM_FREE, je polozka volna
  bool is_directory;    // soubor, nebo adresar
  int8_t references;    // počet odkazů na i-uzel, používá se pro hardlinky
  int file_size;    // velikost souboru v bytech
  int direct1;      // 1. přímý odkaz na datové bloky
  int direct2;      // 2. přímý odkaz na datové bloky
  int direct3;      // 3. přímý odkaz na datové bloky
  int direct4;      // 4. přímý odkaz na datové bloky
  int direct5;      // 5. přímý odkaz na datové bloky
  int indirect1;    // 1. nepřímý odkaz (odkaz - datové bloky)
  int indirect2;    // 2. nepřímý odkaz (odkaz - odkaz - datové bloky)
};

struct directory_item {
  int inode;      // inode odpovídající souboru
  char item_name[12]; // 8+3 + /0 C/C++ ukoncovaci string znak
};

#endif