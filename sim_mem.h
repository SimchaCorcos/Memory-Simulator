#ifndef EX5_SIM_MEM_H
#define EX5_SIM_MEM_H

#define MEMORY_SIZE 100

extern char main_memory[MEMORY_SIZE];
extern int* frames;

typedef struct page_descriptor
{
    unsigned int V; // valid
    unsigned int D; // dirty
    unsigned int P; // permission
    unsigned int frame; //the number of a frame if in case it is page-mapped
} page_descriptor;


class sim_mem {
    int swapfile_fd;    //swap file fd
    int program_fd;     //executable file fd
    int text_size;
    int data_size;
    int bss_size;
    int heap_stack_size;
    int num_of_pages;
    int page_size;
    page_descriptor *page_table; //pointer to page table

public:
    sim_mem(char exe_file_name[], char swap_file_name[], int text_size,
            int data_size, int bss_size, int heap_stack_size,
            int num_of_pages, int page_size);

    ~sim_mem();

    char load(int address);

    void store(int address, char value);

    void print_memory();

    void print_swap();

    void print_page_table();

    static void checkFileOpen(int file, char* name_file);

    static void checkAlloc(void *ptr);

    void readPage(int page, int file_name);

    void checkSwap(int address);
};

#endif //EX5_SIM_MEM_H
