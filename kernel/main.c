#include "kernel/print.h"
#include "init.h"

int main() {
    put_str("kernel initing..:\n");
    init_all();

    while(1);
    return 0;
}