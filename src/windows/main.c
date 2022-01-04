// Analyse von jpeg-Dateien sallomaeander 10.12.21 -23.12.2021

#define _XOPEN_SOURCE 500

#include "untersuche.h"
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64 


int main(int argc, char *argv[]) {
  
if (nftw((argc < 2) ? "." : argv[1], untersuche, 15, FTW_PHYS  ) == -1) {
    perror("nftw");
    exit(EXIT_FAILURE);
  }
    exit(EXIT_SUCCESS);
}
