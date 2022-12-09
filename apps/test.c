void test1() {
    if (access(DISK_PATH, F_OK) == 0) {
        // file doesn't exist
        create_zeroes();
    }

    initFilePtr();
    for (int i = 0; i < BLOCK_SIZE; ++i) {
        buffer_1[i] = 0xe6;
    }
    block_write(buffer_1, 2);
    block_read(buffer_2, 2);
    for (int i = 0; i < BLOCK_SIZE; ++i) {
        printf("%x", buffer_2[i]);
    }
    fclose(ptr);
    return 0;
}

void test2() {
    if (access(DISK_PATH, F_OK) == 0) {
        // file doesn't exist
        create_zeroes();
    }

    initFilePtr();
    for (int i = 0; i < BLOCK_SIZE; ++i) {
        fill_int(buffer_1, i, i, 29L);
    }
    block_write(buffer_1, 2);
    block_read(buffer_2, 2);
    for (int i = 0; i < BLOCK_SIZE; ++i) {
        printf("%x", buffer_2[i]);
    }
    initYLLFS();
    fclose(ptr);
    return 0;
}

void test3() {
    if (access(DISK_PATH, F_OK) == 0) {
        // file doesn't exist
        create_zeroes();
    }

    initFilePtr();
    initYLLFS();

    block_read(buffer_0, 0);
    long val = 0;
    fill_int_read(buffer_0, 4, 7, &val);
    printf("%ld\n", val);
    // close file
    fclose(ptr);
    return 0;
}