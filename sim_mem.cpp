#include "sim_mem.h"

#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>

#define stderr stderr
#define MEMORY_SIZE 100

char main_memory[MEMORY_SIZE];

///which page page in which frame.
int* frames;

int free_frame = 0;
int flag = 0;

sim_mem::sim_mem(char exe_file_name[], char swap_file_name[],
                 int text_size, int data_size, int bss_size, int heap_stack_size, int num_of_pages, int page_size)
{
    int i, j, k, logic_mem_size, wrt;

    ///check that file names are not null.
    if (exe_file_name == nullptr)
    {
        printf("cannot open file, null name\n");
    }

    if(swap_file_name == nullptr)
    {
        printf("cannot open file, null name\n");
    }

    swapfile_fd = open(swap_file_name, O_RDWR | O_CREAT, 0600);///open swap_file, create and permissions.
    checkFileOpen(swapfile_fd, swap_file_name);

    program_fd = open(exe_file_name, O_RDONLY);///open exec_file, read permissions.
    checkFileOpen(program_fd, exe_file_name);

    ///Initialize all variables
    this->text_size = text_size;
    this->data_size = data_size;
    this->bss_size = bss_size;
    this->heap_stack_size = heap_stack_size;
    this->num_of_pages = num_of_pages;
    this->page_size = page_size;

    this->page_table = (page_descriptor*)malloc(sizeof(page_descriptor)*num_of_pages);///allocate memory for page_table.
    checkAlloc(this->page_table);

    ///Initialize all variables page_table.
    for (i = 0;  i < this->num_of_pages ; i++)
    {
        page_table[i].V = 0;
        page_table[i].D = 0;

        if(i < (this->text_size / this->page_size))
        {
            this->page_table[i].P = 0;///in data area permissions are read only.
        }
        else if(i >= (this->text_size / this->page_size))
        {
            this->page_table[i].P = 1;
        }

        this->page_table[i].frame = -1;
    }

    for (j = 0; j < MEMORY_SIZE; j++) ///main memory with 0.
    {
        main_memory[j] = '0';
    }

    logic_mem_size = this->page_size * this->num_of_pages;

    for (k = 0; k < logic_mem_size; k++)
    {
        wrt = (int)write(swapfile_fd, "0", 1); ///swap with 0.
        if(wrt == -1)
        {
            perror("Error: write failed\n");
            exit(EXIT_FAILURE);
        }
    }

    frames = (int*)malloc(sizeof(int)*(MEMORY_SIZE/page_size));///allocate memory for frames array.
    checkAlloc(frames);
}

sim_mem::~sim_mem()
{
    ///close files.
    close(swapfile_fd);
    close(program_fd);

    if(page_table != nullptr) ///free page_table allocate.
    {
        free(page_table);
    }
    if(frames != nullptr) ///free frames allocate.
    {
        free(frames);
    }
}

/*
 * Trying to access a specific address for reading.
 */
char sim_mem::load(int address)
{
    int num_addresses, page, offset, frame, mem_index, bss_start, heap_stack_end, data_start, data_end;
    char c = '\0';

    num_addresses = num_of_pages * page_size -1;
    if(address > num_addresses || address < 0 )///check if address exist.
    {
        fprintf(stderr, "Address does not exist\n");
        return c;
    }

    page = address / this->page_size;
    offset = address % this->page_size;

    if(page_table[page].V == 1)///if valid, return val.
    {
        frame = (int)page_table[page].frame;
        mem_index = (frame * this->page_size) + offset;
        c = main_memory[mem_index];
        return c;
    }

    if(free_frame == (MEMORY_SIZE / page_size))///if main memory full, free_frame - 0 , flag up.
    {
        free_frame = 0;
        flag = 1;
    }

    if(flag == 1)///main memory full, need to check if replace pages to swap or don't need.
    {
        checkSwap(address);
    }

    if(page_table[page].V == 0)
    {
        if(page_table[page].P == 0)///in text, permission is read, so read from exec_file.
        {
            readPage(page, program_fd);///load page from program_fd to main memory.
            page_table[page].V = 1;
            page_table[page].frame = free_frame;
            frames[free_frame] = page;
            c = main_memory[(page_size * free_frame) + offset];///get value and return it.
            free_frame++;

            return c;
        }

        if(page_table[page].P == 1)
        {
            if(page_table[page].D == 1)///if dirty, read from swap.
            {
                readPage(page, swapfile_fd);///load page from program_fd to main memory.

                page_table[page].frame = free_frame;
                frames[free_frame] = page;
                page_table[page].V = 1;
                c = main_memory[page_size * free_frame + offset];
                free_frame++;

                return c;
            }

            bss_start = text_size+data_size;
            heap_stack_end = text_size+data_size+bss_size+heap_stack_size-1;

            if(page_table[page].D == 0)
            {
                if(address >= bss_start && address <= heap_stack_end)///dynamic or locally variables in func. can't load before alloc, error.
                {
                    fprintf(stderr, "Error: can not load non-existent address\n");
                    return c;
                }

                data_start = text_size;
                data_end = text_size+data_size -1;

                if(address >= data_start && address <= data_end)///not dirty, in data.
                {
                    readPage(page, program_fd);///load page from program_fd to main memory.
                    page_table[page].V = 1;
                    page_table[page].frame = free_frame;
                    frames[free_frame] = page;
                    c = main_memory[page_size * free_frame + offset];///get value and return it.

                    free_frame++;
                    return c;
                }
            }
        }
    }
    return c;
}

/*
 * Trying to access a specific address for writing.
 */
void sim_mem::store(int address, char value)
{
    int num_addresses, page, offset, frame, mem_index, bss_start, heap_stack_end, data_start, data_end, i_dynamic;
    char* page_dynamic_s;

    num_addresses = num_of_pages * page_size -1;
    if(address > num_addresses || address < 0 )///check if address exist.
    {
        fprintf(stderr, "Address does not exist\n");
        return;
    }

    page = address / this->page_size;
    offset = address % this->page_size;

    if(page_table[page].V == 1)///if valid put value and return.
    {
        frame = (int)page_table[page].frame;
        mem_index = (frame * this->page_size) + offset;
        main_memory[mem_index] = value;
        page_table[page].D = 1;
        return;
    }

    if(free_frame == (MEMORY_SIZE / page_size))///if main memory full, free_frame - 0 , flag up.
    {
        free_frame = 0;
        flag = 1;
    }

    if(flag == 1)///main memory full, need to check if replace pages to swap or don't need.
    {
        checkSwap(address);
    }

    if(page_table[page].V == 0)
    {
        if(page_table[page].P == 0)///text, permission is read, so can't change values.
        {
            fprintf( stderr,"Error: permission diny\n");
            return;
        }

        if(page_table[page].P == 1)
        {
            if(page_table[page].D == 1)///in swap
            {
                readPage(page, swapfile_fd);///load page from swapfile_fd to main memory.

                page_table[page].frame = free_frame;
                frames[free_frame] = page;
                page_table[page].V = 1;
                page_table[page].D = 1;
                free_frame++;
            }

            bss_start = text_size+data_size;
            heap_stack_end = text_size+data_size+bss_size+heap_stack_size-1;

            if(page_table[page].D == 0)
            {
                if(address >= bss_start && address <= heap_stack_end)///dynamic or locally variables in func.
                {
                    page_dynamic_s = (char*)malloc(sizeof(char)* this->page_size);
                    checkAlloc(page_dynamic_s);

                    for (i_dynamic = 0; i_dynamic < this->page_size; i_dynamic++)///new pages with '0'.
                    {
                        page_dynamic_s[i_dynamic] = '0';
                    }

                    strncpy(&main_memory[free_frame * page_size], page_dynamic_s, page_size);///put '0' page in main memory.
                    page_table[page].V = 1;
                    page_table[page].frame = free_frame;
                    frames[free_frame] = page;
                    main_memory[page_size * free_frame + offset] = value;///put value.
                    page_table[page].D = 1;
                    free_frame++;
                    free(page_dynamic_s);
                    return;
                }

                data_start = text_size;
                data_end = text_size+data_size -1;

                if(address >= data_start && address <= data_end)///address in data.
                {
                    readPage(page, program_fd);///load page from program_fd to main memory.
                    page_table[page].V = 1;
                    page_table[page].frame = free_frame;
                    frames[free_frame] = page;
                    page_table[page].D = 1;
                    main_memory[page_size * free_frame + offset] = value;///put value.

                    free_frame++;
                    return;
                }
            }
        }
    }
}

/*
 * print main memory
 */
void sim_mem::print_memory()
{
    int i;
    printf("\n Physical memory\n");
    for(i = 0; i < MEMORY_SIZE; i++) {
        printf("[%c]\n", main_memory[i]);
    }
}

/*
 * print swap.
 */
void sim_mem::print_swap()
{
    char* str = (char*)malloc(this->page_size *sizeof(char));
    int i;
    printf("\n Swap memory\n");
    lseek(swapfile_fd, 0, SEEK_SET); // go to the start of the file
    while(read(swapfile_fd, str, this->page_size) == this->page_size)
    {
        for(i = 0; i < page_size; i++)
        {
            printf("%d - [%c]\t", i, str[i]);
        }
        printf("\n");
    }
    free(str);
}

/*
 * print page table
 */
void sim_mem::print_page_table()
{
    int i;
    printf("\n page table \n");
    printf("Valid\t Dirty\t Permission \tFrame\n");
    for (i = 0; i < num_of_pages; i++)
    {
        printf("[%d]\t[%d]\t    [%d]\t\t [%d]\n",
               page_table[i].V,
               page_table[i].D,
               page_table[i].P,
               page_table[i].frame);
    }
}

/*
 * Check if file opened
 */
void sim_mem::checkFileOpen(int file, char* name_file)
{
    if(file == -1)
    {
        printf("Error: can not open file. %s\n", name_file); //fail message to user
        exit(EXIT_FAILURE);//error
    }
}

/*
 * Check if malloc succeeded
 */
void sim_mem::checkAlloc(void* ptr)
{
    if(ptr == nullptr)
    {
        perror("Error: malloc failed\n");//fail message to user
        exit(EXIT_FAILURE);//error
    }
}

/*
 * read page from file- program_fd or swap_fd
 */
void sim_mem::readPage(int page, int file_name)///read page from file- program_fd or swap_fd
{
    int num_chars, read_status;
    num_chars = (int)lseek(file_name, page_size * page, SEEK_SET);///put pointer at the beginning of the file.

    if(num_chars == -1)
    {
        perror("Error: lseek failed\n");
        exit(EXIT_FAILURE);
    }

    read_status = (int)read(file_name, &main_memory[free_frame * page_size], page_size);///read from file to main memory.

    if(read_status == -1)
    {
        perror("Error: read failed\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * check if replace to swap or not. and replace if needed.
 */
void sim_mem::checkSwap(int address)
{
    int write_status, num_chars;
    char* mem_page;

    if(address >= text_size && page_table[frames[free_frame]].D == 1)///if dirty, and not in text, need to replace to swap file.
    {
        mem_page = (char*)malloc(sizeof(char)*page_size);
        checkAlloc(mem_page);

        strncpy(mem_page, &main_memory[free_frame * page_size], page_size);///copy from main memory to buff - mem_page.

        num_chars = (int)lseek(swapfile_fd, page_size * frames[free_frame], SEEK_SET);///put pointer at the beginning of the file.

        if(num_chars == -1)
        {
            perror("Error: lseek failed\n");
            exit(EXIT_FAILURE);
        }

        write_status = (int)write(swapfile_fd, mem_page, page_size);///write mem_page to swap.

        if(write_status == -1)
        {
            perror("Error: write failed\n");
            exit(EXIT_FAILURE);
        }

        page_table[frames[free_frame]].V = 0;///Updates dirty and frame.
        page_table[frames[free_frame]].frame = -1;

        free(mem_page);
    }

    ///Updates dirty and frame.
    page_table[frames[free_frame]].V = 0;
    page_table[frames[free_frame]].frame = -1;
}