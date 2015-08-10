#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "buspirate.h"

int outft;

void do_menu(char c) {
  write(outft, &c, 1);
  write(outft, "\n", 1);

  
  while (c != '>') {
    if (read(outft, &c, 1)) {
      printf("%c", c);
    }
  }
}

void spi_wait(int n) {}
// do an SPI transaction and put results in the input buffer
void spi_txn_noreply(unsigned char* b, int n) {
  spi_txn(b,n);
}
void spi_txn(unsigned char* b, int n) {
  write(outft, "{", 1);
  
  int i;
  for (i = 0; i<n; i++) {
    printf("%02X", b[i]);
    write(outft, " ", 1);
    unsigned char d = b[i];
    char c;
    c=d/100 + '0';
    if (d/100)
      write(outft, &c, 1);
    c=(d%100)/10 + '0';
    if ((d/100) || (d/10))
      write(outft, &c, 1);
    c=(d%10) + '0';
    write(outft, &c, 1);
  }
  write(outft, "]\n", 2);

  unsigned char c = '0';
  i=-1;
  int charnum = 10;
  while (c != '>') {
    if (read(outft, &c, 1)) {
      //printf("%c", c);
      charnum++;
      if (c == 'x') {
        i++;
        charnum = 0;
      }
      if (charnum == 1 || charnum == 2) {
        if (c<='9') 
          c-= '0';
        else
          c-= 'A' - 10;

        b[i/2] = (b[i/2]<<4) + c;
      }
    }
  }

  printf(" ");
  for (i = 0; i<n; i++)
    printf("%02X", b[i]);
  printf("\n");
  
}

void CE_lo()
{
  do_menu('a');
}
void CE_hi()
{
  do_menu('A');
}

void spi_init() {
  if((outft = open("/dev/ttyUSB0", O_RDWR))==-1){
    perror("open");
  }

  do_menu('#');
  do_menu('m');
  do_menu('5');
  do_menu('3');
  do_menu('1');
  do_menu('2');
  do_menu('1');
  do_menu('2');
  do_menu('2');

  do_menu('W');

}

