// Analyse von jpeg-Dateien sallomaeander 10.12.21 -23.12.2021

#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <string.h>
#include <byteswap.h>
#include <errno.h>
#include <ftw.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
 
// pfad, funktion,  



int untersuche(const char*, const struct stat*, int, struct FTW*);
 
