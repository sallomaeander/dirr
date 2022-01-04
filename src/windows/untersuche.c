// Analyse von jpeg-Dateien sallomaeander 10.12.21 -23.12.2021

#include "untersuche.h"
#include "search.h"

#define _GNU_SOURCE 

#define MAXTAGS 100
#define MAXIFDS 100

#define ALLES_PALETTI 0
#define INVALID_PATH -3
#define FILE_SHORT -4
#define NO_JPEG -5
#define XMP_MAX 10000

int untersuche(const char *pfadname, const struct stat *sb, int tflag, struct FTW *ftwbuf) {

    char* gef;
  
    struct exifheader_typ {
        unsigned short typ;     // 16 Bit muss FF E1 enthalten
        unsigned short laenge;  // 16 Bit muss die Laenge des EXIF-Headers enthalten, little endian !
        char exifstring[6];     // hier muss "Exif" mit abschließender 0 eingetragen sein
        unsigned short intmot;  // 16 Bit enthaelt 49 49 fuer Intel oder 4D 4D fuer Motorola
        char zwA[6];            // enthaelt bei Intel 2A 00 08 00 00 00, bei Motorola 
                                // 00 2A 00 00 00 08
        unsigned short anzeintr;// 16 Bit Anzahl der 12 Bytes langen Eintraege
    };

    struct ifd_eintrag_typ {    // Eintrag im Image File Directory immer 12 Bytes lang
        unsigned short typ;     // 16 Bit Typ des IFF-Eintrags, z.B. 0F 01 fuer Hersteller der Kamera
        unsigned short datentyp;// 16 Bit Datentyp des Eintrags, z.B. 2 = ASCII-String
        unsigned int laenge;    // 32 Bit Laenge der Daten des Eintrages <=4 heisst direkt in daten
        unsigned int daten;     // 32 Bit Verweis auf Daten oder Daten selbst wenn kleiner gleich 4 Bytes
    };    
    
    struct ifd_eintrag_typ ifd_eintrag[MAXIFDS];
    
    unsigned short testweise;
    
    struct tag_type {
        unsigned short typ;     // 16 Bit Typ des Tags
        unsigned short laenge;  // 16 Bit Laenge des Tags little endian
        fpos_t  fpos;           // Dateiposition des Tags
    };    
    
    FILE* file;                 // Dateizeiger
    struct tag_type tag[MAXTAGS]; 
    struct exifheader_typ exifheader;
    unsigned short magicbyte;   // Marker fuer JPEG FF D8
    int count=0;
    unsigned short ff = 0x00FF; // Maske um festzustellen, ob es sich um ein TAG handelt
				// Kontrollvariablen fuer for-Schleifen
    int contr, contr2, contr3, contr4, contr5, contr6, contr7;          
    int ISMOTOROLA = 0;
    char exifstring[] = "Exif"; // nur wenn beim Tag EF E1 "Exif" eingetragen ist, ist es der gesuchte Tag
    char adobstring[] = "http:";// Der Anfang von Adobes XMP Eintrag. Exif hat zwei Nullen, daher hier 5 Zeichen
    fpos_t exif_offset;         // speichert den Beginn des EXIf, da die Adressen relativ dazu sind
    int ifd_index = 0;          // Variable, die als Index auf alle IFD-Eintraege fungiert
    int ifd_index_sicher;	// sicher ifd_index, weil noch werte angehaengt werden
    unsigned int next_ifd  = 0; // 32 Bit Adresse nach dem letzten ifd. Mit 0 initialisieren, fuer ersten Durchlauf
    unsigned int subifdadr;	// 32 Bit Adresse, an der der subIFD zu finden ist
    unsigned short subifdcount;	// 16 Bit Anzahl der subIFD-Eintraege
    unsigned short serialadr;	// 32 Bit Adresse der Seriennummer
    char serialnumber[8];	// Hier wird ggf. die Seriennummer gespeichert
    
    long offset_tiff = 0;	// 32 Bit Hier wird der Offset des Exif-Headers relativ zum Dateianfang gespeichert
    unsigned short xmp_length;	// TAG-Eintrag, wie lang die XMP-Daten sind
    unsigned char xmp_data[XMP_MAX];	// Feld, um den XMP-Eintrag einzulesen
				// die gesuchte Seriennummer 
    unsigned char* GESUCHT = "6206059";	
    int gefunden = 0;		// wird 1, wenn GESUCHT gefunden wurde


// Ende Variablen, Anfang Programmcode ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    
    // die uebergebenen Flags auswerten
    
    if (tflag == FTW_D) 	return 0;
    if (tflag == FTW_NS) 	return 0;
    if (tflag == FTW_DNR) 	return 0;
    
    errno = 0;
    file = fopen(pfadname, "r");

    if (file == NULL) return 0;
   
    // Zuerst prüfen, ob die ersten zwei Bytes die Dateisignatur eines JPEG enthalten 0xFF, 0xD8; sonst Ende
    
    if (fread(&magicbyte, 1, 2, file) == 0 ) { fclose(file); return 0; }
    if (magicbyte != 0xD8FF) { fclose(file); return 0; }

    // Nun feststellen, ob und wenn ja wo und wie viele JPEG-Tags in dem File enthalten sind. 
    
    while((ff == 0x00FF) && (count < MAXTAGS)) {
        
        fgetpos(file, &tag[count].fpos);
        fread(&(tag[count].typ), 1, 4 , file); // Typ und Laenge auf einen Rutsch lesen
        tag[count].laenge = __bswap_16(tag[count].laenge); // Byteorder drehen, denn die Laenge ist
        // immer in der Folge LH LL angegeben (little endian)
        
        // ermitteln, ob es sich um einen Tag handelt
        ff = (tag[count].typ & 0x00FF);
        
        if (ff != 0x00FF) break;
       
        // Dateizeiger entsprechend der ermittelten Laenge des Tags vorruecken 
        
        fseek(file, (tag[count].laenge - 2),SEEK_CUR);
        count++;
    }



    // Wir interessieren uns hier nur fuer die EXIF-Tags mit der Kennung FF E1
    // Ab hier werden nur die E1 FF Tags bearbeitet! 
    
    for(contr = 0; contr < count; contr++  ){ // fuer jeden JPEG-TAG checken wir die Eintraege
        
        // nur bearbeiten wenn es EXIF sind
        
        if ( tag[contr].typ == 0x0E1FF ){ // es ist ein EXIF-Tag ...

            // wir bewegen den Dateizeiger an den Beginn des Tags...
        
            fsetpos(file, &tag[contr].fpos); // positionieren relativ zum Dateianfang
	    
	    if (offset_tiff == 0) {offset_tiff = ftell(file); offset_tiff += 0xA;} // Nur beim ersten Duruchlauf!
            fread(&exifheader, 1, 20, file);
                            
            //nur wenn "Exif" im Header steht, ist es auch wirklich der gesuchte EXIF-Header
            
            if (strcmp(exifstring, exifheader.exifstring) == 0) { // es ist wirklich der gesuchte Tag mit "Exif"
                
                // die Startadresse des Exif-Headers sichern
                // fgetpos(file, &exif_offset);    
                // Damit man den Offset ab dem Exif-Header berechnen kann!
               
                exifheader.laenge = __bswap_16(exifheader.laenge);  // Motorola Format zu Intel umwandeln
                if (exifheader.intmot == 0x4d4d) ISMOTOROLA = 1;    // wenn little endian = 1, intel = 0
                if (ISMOTOROLA == 1) exifheader.anzeintr = __bswap_16(exifheader.anzeintr);

		// jetzt die ermittelte Zahl der Einträge mal über den EXIF-Header iterieren und die IFDs einlesen
                
                for( contr2 = 0; contr2 < exifheader.anzeintr; contr2++) { // jetzt immer schoen 12 Bytes einlesen
                        
                        fread(&ifd_eintrag[ifd_index],1,12, file);
                        if (ISMOTOROLA) ifd_eintrag[ifd_index].typ = __bswap_16(ifd_eintrag[ifd_index].typ);
                        ifd_index++;
                }
                
                // nach dem letzten ifd eintrag folgt eine 32 bit adresse zum nächsten ifd ...
                fread(&next_ifd, 1, 4, file);
                if (ISMOTOROLA) next_ifd = __bswap_32(next_ifd);
                if (next_ifd == 0) { fclose(file); return 0; }
                if (next_ifd != 0) next_ifd += offset_tiff; // alle Adressen sind relativ zum TIFF-Header ...
                //printf("Die Adresse zum nächsten ifd lautet %X\n", next_ifd);

                // jetz noch die Eintrege aus dem IFD 1 (zweites IFD) auslesen. Die Variable ifd_index wird weitergezaehlt,
                // so dass das Array von ifd-Eintraegen danach ausgewertet werden kann.
               
                fseek(file, next_ifd, SEEK_SET);
                fread(&exifheader.anzeintr, 1, 2 ,file);
                if (ISMOTOROLA) exifheader.anzeintr = __bswap_16(exifheader.anzeintr);
                
                for( contr2 = 0; contr2 < exifheader.anzeintr; contr2++) { // jetzt immer schoen 12 Bytes einlesen
                        
                        fread(&ifd_eintrag[ifd_index],1,12, file);
                        if (ISMOTOROLA) ifd_eintrag[ifd_index].typ = __bswap_16(ifd_eintrag[ifd_index].typ);
                        ifd_index++;
                }


                // jetzt subIFD(s) einlesen und die Werte an die ifd-eintragsliste anhaengen. Dazu muss ifd_index in 
                // eine andere Variable uebertragen werden, denn wir haengen ja eintraege hinten an die liste und erhoehen
                // damit ifd_index 
                
                ifd_index_sicher = ifd_index;
                
                for( contr4 = 0; contr4 < ifd_index_sicher; contr4++) {
                    if (ifd_eintrag[contr4].typ == 0x08769) {
                        subifdadr = ifd_eintrag[contr4].daten;
                        if (ISMOTOROLA) subifdadr = __bswap_32(subifdadr);
                        subifdadr += offset_tiff;
		      
                        // Dateizeiger auf den Wert Anzahl der subIFS-Eintraege bewegen...
		      
                        fseek(file, subifdadr, SEEK_SET); // von Dateianfang an positionieren
                        fread(&subifdcount, 1, 2 ,file); // Einlesen wie viele Eintraege im subIFD
                        if (ISMOTOROLA) subifdcount = __bswap_16(subifdcount);

			// die Eintraege des subIFDs einlesen
                        for(contr5 = 0; contr5 < subifdcount; contr5++ ) {
                            fread(&ifd_eintrag[ifd_index],1,12, file);
                            if (ISMOTOROLA) ifd_eintrag[ifd_index].typ = __bswap_16(ifd_eintrag[ifd_index].typ);
                            ifd_index++;
                        }
                    }  
                }  

            }
            
            // oder, es ist der XMP-Teil von Adobe
            else {

	      exifheader.exifstring[5] = 0;
	      if (strcmp(adobstring, exifheader.exifstring) == 0) {
		fsetpos(file, &tag[contr].fpos); // positionieren relativ zum Dateianfang
		fread(&xmp_length, 1, 2, file);//printf("Erstes Lesen: %X\n", xmp_length);
		fread(&xmp_length, 1, 2, file);//printf("Zweites Lesen: %X\n", xmp_length);
		xmp_length = __bswap_16(xmp_length); // iummer als LH LL gespeichert
		if (xmp_length > XMP_MAX) xmp_length = XMP_MAX; // nicht mehr lesen, als wir Platz haben
		fread(&xmp_data, 1, xmp_length, file);
		gef= (void*)(searchSimple(xmp_data, xmp_length, GESUCHT, strlen(GESUCHT)));
		if (gef != NULL) gefunden = 1;
	      }
	    }  
        }
        
        
    }
    
    // jetzt alle IFD Eintraege nach dem Tag 31 A4 durchsuchen und ggf. die angegebene Adresse auslesen
    
    for ( contr6 = 0; contr6 < ifd_index; contr6++) {
      if (ifd_eintrag[contr6].typ == 0x0A431 ) {
	
	serialadr = ifd_eintrag[contr6].daten;
	if (ISMOTOROLA) serialadr = __bswap_32(serialadr);
	serialadr += offset_tiff;
	
	fseek(file, serialadr, SEEK_SET); // von Dateianfang an positionieren
	fread(&serialnumber, 1, 8, file);
	if (strcmp(serialnumber, GESUCHT) == 0) gefunden = 1;
      }
	  
    }  
    
    
    fclose(file);
    if (gefunden) printf("%s\n", pfadname); 
    return ALLES_PALETTI;
}    

