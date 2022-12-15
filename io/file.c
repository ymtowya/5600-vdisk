#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#define OCCP 0x00
#define FREE 0x01
#define BLOCK_SIZE 512
#define BLOCK_NUM 4096
#define INODE_NUM 4096
#define INODE_SIZE 32
#define INODES_BMAP_BLOCK_ID 1
#define BLOCKS_BMAP_BLOCK_ID 2
#define INODES_BLOCK_ID 3
#define INODE_DIR_LINK_NUM 9
#define INODE_IND_LINK_NUM 1
#define FILE_NAME_MAX_LEN 16
#define DELIM '|'
#define DISK_PATH "./disk.bin" // "./../disk/disk.bin"

unsigned char inode_buff[INODE_SIZE];
unsigned char buffer_0[BLOCK_SIZE];
unsigned char buffer_1[BLOCK_SIZE];
unsigned char buffer_2[BLOCK_SIZE];
unsigned char buffer_3[BLOCK_SIZE];
unsigned char* buffers[4] = {buffer_0, buffer_1, buffer_2, buffer_3};
FILE *ptr;

// |
typedef struct INode
{
    /* data */
    char type;
    int index;
    int blocks;
    int hard_link_num;
    int bytes;
    int direct_links[INODE_DIR_LINK_NUM];
    int indirect_links;
    char name[FILE_NAME_MAX_LEN];
} INode;


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

int get_bit_read(unsigned char *buff, int pos) {
    int i = pos / 8;
    int r = 7 - pos % 8;
    unsigned char tmp = buff[i];
    return (int) 0x01 & (tmp >> r);
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
    fill_bit_write(buffer_1, 0, 9, OCCP); // first 10 blocks not usable
    block_write(buffer_1, BLOCKS_BMAP_BLOCK_ID);
}

int inode_write(unsigned char *buff, int inode_id) {
    // 512 / 32 = 16 inodes in one block
    int block_id = inode_id / 16 + INODES_BLOCK_ID;
    int offset = INODE_SIZE * (inode_id % 16);
    rewind(ptr);
    fseek(ptr, block_id * BLOCK_SIZE + offset, SEEK_SET);
    int re = fwrite(buff, 1, INODE_SIZE, ptr);
    return re;
}

int inode_read(unsigned char *buff, int inode_id) {
    // 512 / 32 = 16 inodes in one block
    int block_id = inode_id / 16 + INODES_BLOCK_ID;
    int offset = INODE_SIZE * (inode_id % 16);
    rewind(ptr);
    fseek(ptr, block_id * BLOCK_SIZE + offset, SEEK_SET);
    int re = fread(buff, 1, INODE_SIZE, ptr);
    return re;
}

unsigned char* getStrOfInode(INode* n) {
    unsigned char* res = (unsigned char *) malloc(INODE_SIZE);
    // type
    res[0] = n->type;
    //index
    res[1] = n->index;
    // blocks
    res[2] = n->blocks;
    // hard links
    res[3] = n->hard_link_num;
    // bytes
    fill_int_write(res, 4, 5, (long) n->bytes);
    // direct links
    for (int i = 0; i < n->blocks; ++i) {
        res[6 + i] = n->direct_links[i];
    }
    // indirect
    res[15] = n->indirect_links;
    // name
    memcpy(res + 16, n->name, 15);
    res[31] = '\0';
    // finish
    return res;
}

INode* getINodeFromStr(unsigned char *s) {
    INode * node = (INode *) malloc(sizeof(INode));
    // type
    node->type = s[0];
    // index
    node->index = (int) s[1];
    // blocks
    node->blocks = (int) s[2];
    // hard links
    node->hard_link_num = (int) s[3];
    // bytes
    long tmp = 0L;
    fill_int_read(s, 4, 5, &tmp);
    node->bytes = (int) tmp;
    // direct
    for (int i = 0; i < node->blocks; ++i) {
        node->direct_links[i] = (int) s[6 + i];
    }
    // indirect
    node->indirect_links = (int) s[15];
    // name
    memcpy(node->name, s + 16, 15);
    node->name[15] = '\0';
    return node;
}

INode* find_free_inode() {
    // read in the inode bitmap
    block_read(buffer_0, INODES_BMAP_BLOCK_ID);
    // go through bitmap
    for (int i = 0; i < INODE_NUM; ++i) {
        // if inode is not occupied
        if (get_bit_read(buffer_0, i) != OCCP) {
            // get the inode
            inode_read(inode_buff, i);
            INode* tmp = getINodeFromStr(inode_buff);
            // set id
            tmp->index = i;
            memcpy(inode_buff, getStrOfInode(tmp), INODE_SIZE);
            inode_write(inode_buff, i);
            return tmp;
        }
    }
    // not found
    return NULL;
}

int search_inode_id(char * file_path) {
    // init config
    if (strlen(file_path) >= FILE_NAME_MAX_LEN) {
        file_path[FILE_NAME_MAX_LEN - 1] = '\0';
    }
    // read in the inode bitmap
    block_read(buffer_0, INODES_BMAP_BLOCK_ID);
    // go through bitmap
    for (int i = 0; i < INODE_NUM; ++i) {
        // if inode is occupied
        if (get_bit_read(buffer_0, i) == OCCP) {
            // get the inode
            inode_read(inode_buff, i);
            INode* tmp = getINodeFromStr(inode_buff);
            // compare file name
            if (strcmp(tmp->name, file_path) == 0) {
                return tmp->index;
            }
        }
    }
    // not found
    return -1;
}

void free_inode(int inode_id) {
    // read into the bitmap
    block_read(buffer_2, INODES_BMAP_BLOCK_ID);
    // set inode bitmap as 1
    set_bit_write(buffer_2, inode_id, FREE);
    // write the bitmap
    block_write(buffer_2, INODES_BMAP_BLOCK_ID);
}

void free_blocks(INode *n) {
    // read into the bitmap
    block_read(buffer_1, BLOCKS_BMAP_BLOCK_ID);
    // set block bitmap as 1
    for (int i = 0; i < n->blocks; ++i) {
        int tmp = n->direct_links[i];
        set_bit_write(buffer_1, tmp, FREE);
    }
    // write the bitmap
    block_write(buffer_1, BLOCKS_BMAP_BLOCK_ID);
}

void myRead(char* file_path, char** content) {

}

INode* myWrite(char* file_path, char* content) {
    int content_len = strlen(content) + 1;
    if (content_len < 1) {
        return;
    }
    int blocks_needed = (int) ceil(content_len / BLOCK_SIZE);
    blocks_needed = (blocks_needed > INODE_DIR_LINK_NUM) ?
                    INODE_DIR_LINK_NUM : blocks_needed;
    INode* n = (INode *) malloc(sizeof(INode));
    n->blocks = blocks_needed;
    int inode_id = search_inode_id(file_path);
    if (inode_id != -1) {
        // TODO: release the whole inode
    }
    inode_id = find_free_inode();
    if (inode_id == -1) {
        // TODO: no more free inodes
    }
    n->index = inode_id;
    n->bytes = content_len;

}

void setTest() {
    if (access(DISK_PATH, F_OK) != 0) {
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
        // printf("%x ", buffer_1[i]);
    }
    // test write inode

    inode_read(inode_buff, 11);
    INode* i = getINodeFromStr(inode_buff);
    i->bytes = 7219;
    memcpy(inode_buff, getStrOfInode(i), INODE_SIZE);
    inode_write(inode_buff, 7);
    // linear search
    block_read(buffer_0, INODES_BMAP_BLOCK_ID);
    set_bit_write(buffer_0, 11, 0);
    block_write(buffer_0, INODES_BMAP_BLOCK_ID);
    int index = search_inode_id("/yopou.txt\0");
    printf("The inode id is : %d\n", index);
    // close file
    fclose(ptr);
}

int main(int argc, char const *argv[])
{
    INode m;
    m.type = 'f';
    m.index = 1;
    m.blocks = 2;
    m.hard_link_num = 1;;
    m.bytes = 3724;
    m.direct_links[0] = 15;
    m.direct_links[1] = 31;
    m.indirect_links = 23;
    memcpy(m.name, "/yopou.txt\0", 11);

    memcpy(inode_buff, getStrOfInode(&m), INODE_SIZE);

    for (int i = 0; i < INODE_SIZE; ++i) {
        printf("%d ", (int) inode_buff[i]);
    }
    printf("\n%s\n", inode_buff + 16);

    setTest();

    return 0;
}

void testBitRead() {
    unsigned char bu[4];
    bu[0] = 0x2B;
    bu[1] = 0x2B;
    printf("%d\n", get_bit_read(bu, 2));
    printf("%d\n", get_bit_read(bu, 5));
    printf("%d\n", get_bit_read(bu, 12));
    printf("%d\n", get_bit_read(bu, 13));
    printf("%d\n", get_bit_read(bu, 15));

}