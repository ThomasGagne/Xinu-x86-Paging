


#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

shellcmd xsh_memsizetest(int nargs, char *args[]) {

  // Attempt to malloc large chunks of memory, starting with 2kb, then 6kb, then 500mb, then 1500mb, then 3000mb

  printf("Malloc'ing 2kb...\n");
  char *test;

  printf("memlist.length: %x\n", memlist.length);

  // 2kb
  test = (char *)malloc(sizeof(char) * 2048);
  if((int)*test != SYSERR) {
    printf("Malloc'd 2kb successfully!\n");
    free(test);
  } else {
    printf("Malloc returned SYSERR :c\n");
  }
  
  printf("memlist.length: %x\n", memlist.length);
    
  // 6kb, larger than 1 page
  test = (char *)malloc(sizeof(char) * 6144);
  if((int)*test != SYSERR) {
    printf("Malloc'd 6kb successfully!\n");
    free(test);
  } else {
    printf("Malloc returned SYSERR :c\n");
  }

  printf("memlist.length: %x\n", memlist.length);

  /*

  // .5GB
  test = (char *)malloc(sizeof(char) * 0x20000000);
  if(test != SYSERR) {
    printf("Malloc'd .5GB successfully!\n");
    free(test);
  } else {
    printf("Malloc returned SYSERR :c\n");
  }

  // 2.5GB, should be too big to fit into userspace!
  test = (char *)malloc(sizeof(char) * 0xFFFFFFFF);
  if(test != NULL) {
    printf("Malloc'd 1.5GB successfully!\n");

    test[0] = 4;
    test[0xA0000000] = 5;
    printf("%d, %d, %d, %d\n", test[0], test[0x40000000], test[0xA0000000], test[0xFFFFFFFE]);

    // Let's do a test on whether or not we can actually write to that much memory
    int i = 0;
    for(i = 0; i < 0x10000000; i += 0x00001000) {
      printf("Setting: %d: %d\n", i, test[i]);
      test[i] = i;
      printf("Setting: %d: %d\n", i, test[i]);
    }
    
    free(test);
  } else {
    printf("Malloc returned SYSERR :c\n");
  }

  */

  return OK;
  
}
