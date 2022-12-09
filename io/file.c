#include <stdio.h>
#include <unistd.h>

#define BLOCK_SIZE 512
#define BLOCK_NUM 4096
#define INODE_SIZE 32
#define INODES_BMAP_BLOCK_ID 1
#define BLOCKS_BMAP_BLOCK_ID 2
#define DISK_PATH "./../disk/disk.bin"

unsigned char inode_buff[INODE_SIZE];
unsigned char buffer_0[BLOCK_SIZE];
unsigned char buffer_1[BLOCK_SIZE];
unsigned char buffer_2[BLOCK_SIZE];
unsigned char buffer_3[BLOCK_SIZE];
unsigned char* buffers[4] = {buffer_0, buffer_1, buffer_2, buffer_3};
FILE *ptr;

int create_zeroes() {
    ptr = fopen(DISK_PATH, "wb");
    if (!ptr) {
        printf("File open failed.\n");
        return -1;
    }
    for (int i = 0; i < BLOCK_SIZE; ++i) {
        buffer_1[i] = 0x00;
    }
    for (int i = 0; i < BLOCK_NUM; ++i) {
        fwrite(buffer_1, 1, BLOCK_SIZE, ptr);
    }
    fclose(ptr);
    return 1;
}

void initFilePtr() {
    ptr = fopen(DISK_PATH, "r+b");
}

int block_read(unsigned char* buffer, int block_id) {
    if (block_id > BLOCK_NUM) {
        printf("block index exceeds.\n");
        return -1;
    }
    rewind(ptr);

    fseek(ptr, block_id * BLOCK_SIZE, SEEK_SET);
    int re = fread(buffer, 1, BLOCK_SIZE, ptr);
    return re;
}

int block_write(unsigned char* buffer, int block_id) {
    if (block_id > BLOCK_NUM) {
        printf("block index exceeds.\n");
        return -1;
    }
    rewind(ptr);
    fseek(ptr, block_id * BLOCK_SIZE, SEEK_SET);
    int re = fwrite(buffer, 1, BLOCK_SIZE, ptr);
    return re;
}

void buff_fills(unsigned char * buff, unsigned char v) {
    for (int i = 0; i < BLOCK_SIZE; ++i) {
        buff[i] = v;
    }
}

void fill_int_write(unsigned char *buff, int start, int end, long value) {
    char flag = 0xff;
    for (int i = end; i >= start; --i) {
        buff[i] = value & flag;
        value = value >> 8;
    }
}

void fill_int_read(unsigned char *buff, int start, int end, long* value) {
    *value = 0;
    for (int i = start; i <= end; ++i) {
        long tmp = buff[i] & 0xff;
        *value = *value << 8;
        *value += tmp;
    }
}

void set_bit_write(unsigned char *buff, int pos, unsigned char value) {
    unsigned char tmp = value & 0x01;
    int i = pos / 8;
    int r = 7 - pos % 8;
    
    if (tmp == 0L) {
        // 0 - &
        tmp = 0x0 | (0x1 << r);
        tmp = ~tmp & 0xff;
        buff[i] &= tmp;
    } else {
        // 1 - |
        tmp = 0x0 | (0x1 << r);
        buff[i] |= tmp;
    }
}

void fill_bit_write(unsigned char *buff, int start, int end, unsigned char value) {
    for (int i = start; i <= end; ++i) {
        set_bit_write(buff, i, value);
    }
}

void initYLLFS() {
    
    // Block 0 - metadata
    buff_fills(buffer_0, 0x0);
    fill_int_write(buffer_0, 0, 3, 0x01); // magic number ?
    fill_int_write(buffer_0, 4, 7, 200L); // block number
    fill_int_write(buffer_0, 8, 11, 4096L); // number of inodes
    block_write(buffer_0, 0);
    // Block 1 - inode bitmap
    buff_fills(buffer_1, 0xff);
    block_write(buffer_1, INODES_BMAP_BLOCK_ID);
    // Block 2 - data bitmap
    buff_fills(buffer_1, 0xff);
    fill_bit_write(buffer_1, 0, 9, 0x0); // first 10 blocks not usable
    block_write(buffer_1, BLOCKS_BMAP_BLOCK_ID);
}

int inode_write(unsigned char *buff, int inode_id) {
    // 512 / 32 = 16 inodes in one block
    int block_id = inode_id / 16 + INODES_BMAP_BLOCK_ID;
    int offset = INODE_SIZE * (inode_id % 16);
    rewind(ptr);
    fseek(ptr, block_id * BLOCK_SIZE + offset, SEEK_SET);
    int re = fwrite(buff, 1, INODE_SIZE, ptr);
    return re;
}

void myRead() {

}

void myWrite() {

}

int main(int argc, char const *argv[])
{
    if (access(DISK_PATH, F_OK) == 0) {
        // file doesn't exist
        create_zeroes();
    }
    // metadata set
    initFilePtr();
    initYLLFS();
    // inode set
    inode_buff[1] = 0x34;
    inode_write(inode_buff, 11);
    // write and read block
    buff_fills(buffer_0, 0x23);
    block_write(buffer_0, 16);
    block_read(buffer_1, 16);
    for (int i = 0; i < BLOCK_SIZE; ++i) {
        printf("%x ", buffer_1[i]);
    }
    // close file
    fclose(ptr);
    return 0;
}
