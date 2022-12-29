#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mdadm.h"
#include "jbod.h"

void translate_address(uint32_t linear_addr, int *disk_num, int *block_num, int *offset){
*disk_num=linear_addr / JBOD_DISK_SIZE;
*block_num=(linear_addr % JBOD_DISK_SIZE) / JBOD_BLOCK_SIZE;
*offset=(linear_addr % JBOD_DISK_SIZE) % JBOD_BLOCK_SIZE;

}


int c=0;
uint32_t encode_operation(jbod_cmd_t cmd, int disk_num,int block_num){
uint32_t op=0;


op=cmd<<26 | disk_num<<22 | block_num;
return op;

}

void READ(uint8_t *buf){
 jbod_client_operation(encode_operation(JBOD_READ_BLOCK,0,0), buf);

}

void WRITE(uint8_t *buf){
jbod_client_operation(encode_operation(JBOD_WRITE_BLOCK,0,0), buf);

}
int mdadm_mount(void) {
	uint32_t op=encode_operation(JBOD_MOUNT, JBOD_SEEK_TO_DISK,JBOD_SEEK_TO_BLOCK);
 	if (jbod_client_operation(op,NULL)==0){
 		c=1;
 		return 1;
	}
	else {
	return -1;
	}
	
}

int mdadm_unmount(void) {
	uint32_t op=encode_operation(JBOD_UNMOUNT, JBOD_SEEK_TO_DISK,JBOD_SEEK_TO_BLOCK);
 	if (jbod_client_operation(op,NULL)==0){
 		c=-1;
 		return 1;
	}
	else {
	return -1;
	}
	
}

void seek(int disk_num, int block_num){
uint32_t op=encode_operation(JBOD_SEEK_TO_DISK,disk_num,0);
jbod_client_operation(op,NULL);
op=encode_operation(JBOD_SEEK_TO_BLOCK,0,block_num);
jbod_client_operation(op,NULL);
}

int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf) {

if (len==0){
 	
 	return 0;
 	}

if (c==-1){
	return -1;

}

//invalid tester
	
 if (addr+len>=1048576){
 return -1;
 
 }
 if ( len>1024 ){
	return -1;
}

 if (len!=0 && buf==NULL){
 
return -1;
 }
 
 
 int disk_num, block_num, offset;
  
  uint8_t buff[256];
  
 
  
  translate_address(addr, &disk_num, &block_num, &offset);
  int n = len+offset, i=0, block_read=0;
  seek(disk_num, block_num);
  int tmpdisk, tmpblock;
  tmpdisk = disk_num;
  tmpblock = block_num;
  
  
while ( n > 0){	
if (block_num+block_read>256){
seek(disk_num+=1,0);
tmpdisk = tmpdisk+1;
      tmpblock = 0;
   }
    if(cache_lookup(tmpdisk, tmpblock, buff) < 0){  
      READ(buff);
      cache_insert(tmpdisk, tmpblock, buff);
    }
tmpblock++;
block_num+=1;
seek(disk_num, block_num);
  if (i== 0){
      
  if (n<256){
memcpy(buf,buff+offset,len);
}
  else{
	  memcpy(buf,buff+offset,256-offset);
	}
	i= 1;
      }
	else if (n<256){
      memcpy(buf+block_read*256-offset,buff,len+offset-block_read*256);
    }
    
    else{
      memcpy(buf+block_read*256-offset,buff,256);
    }
	n-=256;
   block_read++;
  }
  
  return len;
}



int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf) {
 if (len==0){
return 0;
}

if (c==-1){
return -1;
}

//invalid tester
	
 if (addr+len>1048576){
 return -1;
 }
 if ( len>1024 ){
	return -1;
}

 if (len!=0 && buf==NULL){
return -1;
 }
 
  int disk_num, block_num, offset;
  int tmpdisk, tmpblock;
  uint8_t buff[256]={};
  
  translate_address(addr, &disk_num, &block_num, &offset);
  
  tmpdisk=disk_num;
  tmpblock=block_num;
  seek(tmpdisk,tmpblock);
  
  int n = offset+len, i= 0, block_read = 0;
  while (n>0){
    if(tmpblock>255){
       tmpdisk++;
      tmpblock = 0;
      seek(tmpdisk,tmpblock);
      }
      READ(buff);
   

if (i==0){

	if(n > 256){
 memcpy(buff+offset,buf,256-offset);
 seek(tmpdisk, tmpblock);
 WRITE(buff);
 cache_update(tmpdisk, tmpblock, buff);
 cache_insert(tmpdisk, tmpblock, buff);
 tmpblock++;
} 
	else{
 seek(tmpdisk,tmpblock);
 memcpy(buff+offset,buf,len);
 WRITE(buff);
 cache_update(tmpdisk, tmpblock, buff);
 cache_insert(tmpdisk, tmpblock, buff);
}

i=1;
      
}
    
else if(n<256){
     memcpy(buff,buf+block_read*JBOD_BLOCK_SIZE-offset,offset+len-block_read*JBOD_BLOCK_SIZE);
     seek(tmpdisk,tmpblock);
     WRITE(buff);
   cache_update(tmpdisk, tmpblock, buff);
 cache_insert(tmpdisk, tmpblock, buff);
     tmpblock++;
}

else{
	memcpy(buff,buf+block_read*JBOD_BLOCK_SIZE - offset,JBOD_BLOCK_SIZE);
	seek(tmpdisk,tmpblock);
	WRITE(buff);
	cache_update(tmpdisk, tmpblock, buff);
 cache_insert(tmpdisk, tmpblock, buff);
	tmpblock++;
    }
    
    n-=256;
     block_read++;
     
}
 return len;
 }
 
