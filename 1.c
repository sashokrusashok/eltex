#include <stdio.h>
#include <stdlib.h>
#include "1.h"
int a(void) {
  int i=0,g=0;
  while(i++<100000)
  {
     g+=i;
  }
  return g;
}
int b(void) {
  int i=0,g=0;
  while(i++<400000)
  {
    g+=i;
  }
  return g;
}
 
int main(int argc, char** argv)
{
   int iterations;
 
   if(argc != 2)
   {
      printf("Usage %s <No of Iterations>\n", argv[0]);
      exit(-1);
   }
   else
      iterations = atoi(argv[1]);
 
   printf("No of iterations = %d\n", iterations);
 
   while(iterations--)
   {
      a();
      b();
   }
   er();
   return 0;
}
//бинарные опzxcvерации
/*#include <stdio.h>
int main()
{
	int a = 1770;
	int b;
	//b=(a<<4)>>4;

	//b = b & 0xFF0;
	printf(" %d \n",b= a & 0x070);
	return 0;xcvbvcxbvcxb
}*/