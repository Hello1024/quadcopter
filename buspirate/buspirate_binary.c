#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "buspirate.h"

int outft;
int byte_counter;

void recv(unsigned char* d, int n) {   // flush out the buffers and get the last n bytes.
  unsigned char buf[byte_counter];
  int done = 0;

  int new_n = n+(n-1)/16;  // to include acks

  while (done != byte_counter)
    done += read(outft, buf+done, byte_counter-done);

 /* int j;
  for (j=0; j<done; j++)
    printf("%02X", buf[j]);
  printf("\n");
  */
  int i;
  for (i=0; i<n; i++) {
    d[i] = buf[byte_counter-new_n+i+(i)/16];
  }
  byte_counter -= done;
}


void send(unsigned char d, int bytes) {
  write(outft, &d, 1);
  byte_counter += bytes;
}

void do_init() {
  unsigned char c=0;
  char kInit[] = "SPI1";

  int i;
  for(i=0; i<20; i++)
    send(0,0);

  send(0x01,0);

  i=0;
  while (i != 4) {
    if (read(outft, &c, 1)) {
      if (c==kInit[i]) i++; else i=0;
      printf("%c", c);
    }
  }
  printf("\n");
  byte_counter=0;
  send(0b10001010, 1);
  send(0b01100010, 1);
  send(0b01001001, 1);

}


void subsend(unsigned char* b, int n) {
  if (n>16) {
    subsend(b, 16);
    subsend(b+16, n-16);
  } else {
    send(0b10000 + n - 1, 1);
    int i;
    for (i=0; i<n; i++) {
      send(b[i], 1);
    }
  }
}

// do an SPI transaction and put results in the input buffer
void spi_txn(unsigned char* b, int n) {
  int i;
  
  send(0b10, 1);
  subsend(b,n);
  recv(b,n);
  send(0b11, 1);

  
}

// do an SPI transaction and put results in the input buffer
void spi_txn_noreply(unsigned char* b, int n) {
  int i;
 // for (i=0; i<n; i++)
 //   printf("%02X", b[i]);
 // printf("\n");
  
  send(0b10, 1);
  subsend(b,n);
  // recv(b,n);
  send(0b11, 1);

//  for (i=0; i<n; i++)
//    printf("%02X", b[i]);
//  printf("\n");
  
}

void CE_lo()
{
  //spi_wait(20);
  send(0b01001001, 1);
  //spi_wait(20);
}
void CE_hi()
{
  //spi_wait(20);
  send(0b01001011, 1);
  //spi_wait(20);
}

void spi_wait(int n)
{
  char c;
  while(n--) {
    send(0b01100010, 1);
    recv(&c, 0);
  }

}

void spi_init() {
  if((outft = open("/dev/ttyUSB0", O_RDWR))==-1){
    perror("open");
  }
  do_init();
  


}

