/* truncate the log; manually inspect to see that the log has shrunk
 * to nothing */

#include "rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) 
{
     rvm_t rvm;

     printf("Before Truncation:\n");
     system("ls -l /media/DATA/linuxworkspace/RVM");
     
     rvm = rvm_init("/media/DATA/linuxworkspace/RVM");
     rvm_truncate_log(rvm);

     printf("\nAfter Truncation:\n");
     system("ls -l /media/DATA/linuxworkspace/RVM");

     return 0;
}
