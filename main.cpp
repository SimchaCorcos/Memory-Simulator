#include <iostream>

#include "sim_mem.h"

using namespace std;
int main()
{
    char val;
    sim_mem mem_sm((char *)"exec_file", (char *)"swap_file", 25, 50, 25, 25, 25, 5);

    val = mem_sm.load(120);
    cout << val << std::endl;

    val = mem_sm.load(0);
    cout << val << std::endl;

    val = mem_sm.load(5);
    cout << val << std::endl;

    val = mem_sm.load(200);
    cout << val << std::endl;

    val = mem_sm.load(10);
    cout << val << std::endl;

    val = mem_sm.load(15);
    cout << val << std::endl;

    mem_sm.store(40, 'm');

    val = mem_sm.load(20);
    cout << val << std::endl;

    mem_sm.store(85, ';');

    val = mem_sm.load(25);
    cout << val << std::endl;

    val = mem_sm.load(30);
    cout << val << std::endl;

    val = mem_sm.load(35);
    cout << val << std::endl;

    val = mem_sm.load(40);
    cout << val << std::endl;

    val = mem_sm.load(90);
    cout << val << std::endl;

    val = mem_sm.load(95);
    cout << val << std::endl;

    mem_sm.store(90, ')');

    mem_sm.store(120, '>');

    mem_sm.store(119, '^');

    mem_sm.store(50, '$');

    mem_sm.store(70, '#');

    mem_sm.store(75, '+');

    val = mem_sm.load(95);
    cout << val << std::endl;

    val = mem_sm.load(90);
    cout << val << std::endl;

    val = mem_sm.load(50);
    cout << val << std::endl;

    mem_sm.store(51, '+');

    mem_sm.print_memory();
    mem_sm.print_page_table();
    mem_sm.print_swap();

    return 0;
}
