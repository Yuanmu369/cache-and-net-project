#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cache.h"

static cache_entry_t *cache = NULL;
static int cache_size = 0;
static int clock = 0;
static int num_queries = 0;
static int num_hits = 0;

int cache_create(int num_entries) {
  if(cache){
    return -1;
  }
  // check for inviald testers
  if(num_entries < 2 || num_entries >4096){
      return -1;
  }
  cache_size = num_entries;
  //initialize cache
  cache = (cache_entry_t *) calloc(num_entries,sizeof(cache_entry_t));
  if (cache==NULL){
      return -1;
  }
  return 1;
}
//Close the cache, clean up remaining data
int cache_destroy(void) {
  if(cache == NULL){
    return -1;
  }
  free(cache);
  cache = NULL;
  return 1;
}

int cache_lookup(int disk_num, int block_num, uint8_t *buf) {
  int i;
  // check for inviald testers
  if(cache == NULL || buf == NULL){
    return -1;
  }
  num_queries++;
  //Lookup the block identified by disk_num and block_num in the cache
  for(i=0; i<cache_size; i++){
  //If found, copy the block into buf
    if(cache[i].disk_num==disk_num && cache[i].block_num==block_num && cache[i].valid){
      num_hits++;
      clock++;
      cache[i].access_time = clock;
      memcpy(buf, cache[i].block, JBOD_BLOCK_SIZE*sizeof(uint8_t));
      return 1;
    }
  }
  return -1;
}

void cache_update(int disk_num, int block_num, const uint8_t *buf) {
  int i;
  // check for inviald testers
  if(cache == NULL || buf == NULL){
    return;
  }
  //Lookup the block identified by disk_num and block_num in the cache
  for(i=0; i<cache_size; i++){
  // If the entry exists in cache, updates its block content with the new data in buf.
    if(cache[i].disk_num==disk_num 
       && cache[i].block_num==block_num 
       && cache[i].valid){
      clock++;
      cache[i].access_time = clock;
      memcpy(cache[i].block, buf, JBOD_BLOCK_SIZE*sizeof(uint8_t));
    }
  }
  
}

int cache_insert(int disk_num, int block_num, const uint8_t *buf) {
  int i;
  int index = 0;
  // check for inviald testers
  if(cache == NULL || buf==0){
    return -1;
  }
  if(disk_num<0 || disk_num >= JBOD_NUM_DISKS){
      return -1;
  }
  if(block_num<0 || block_num >= JBOD_BLOCK_SIZE){
      return -1;
  }
  for(i=0; i<cache_size; i++){
      if(cache[i].valid && cache[i].disk_num==disk_num && cache[i].block_num==block_num){
          return -1;
      }
  }

  for(i=0; i<cache_size; i++){
    if(!cache[i].valid){
      index = i;
      break;
    }else if(cache[i].access_time < cache[index].access_time){
      index = i;
    }
  }

  clock++;
  // Insert the block identified by disk_num and block_num into the cache and copy buf
  memcpy(cache[index].block, buf, JBOD_BLOCK_SIZE*sizeof(uint8_t));
  cache[index].valid = true;
  cache[index].disk_num = disk_num;
  cache[index].block_num = block_num;
  cache[index].access_time = clock;
  
  return 1;
}

bool cache_enabled(void) {
  return cache!=NULL;
}

void cache_print_hit_rate(void) {
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float) num_hits / num_queries);
}