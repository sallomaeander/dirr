# dirr
Search local disks for pictures made by Nikon D3300 camera S/N 6206059 as used by German child abuser

This program is aimed at unveiling the identity of a German child abuser, see 
https://www.bka.de/DE/IhreSicherheit/Fahndungen/Personen/UnbekanntePersonen/Schwerer_Kindesmissbrauch/Sachverhalt.html

Description in German is in *.txt files. (Beschreibung auf Deutsch ist in den *.txt-Dateien).

The perpetrator has likely used a camera of make Nikon D3300 with S/N 6206059 . Unlike many other digital image cameras, the
Nikon D3300 stores the S/N of the body in the EXIF Header. Furthermore, the S/N is stored in an Adobe area at least a second time.

The program I (sallomaeander) provide is doing a tree walk via nftw in the main.c part of the programme. In the part called untersuche.c the routine opens any existing physical file, checks the first two bytes (magic bytes) so as to test if it is a JPEG file, thus ignoring the file's name or ending.

If it ist a jpeg, the program walks through the file's JPEG and EXIF Tags to find the Serial Number. It also checks the Adobe extension where there might be a second instance of the same number. 

The program is byte-order aware. So, if pictures have been processed on, say, an old Macintosh, the program will still be able to inspect them.

The output of the program is none, if nothing is found. If an image with the S/N of the child abuser's camera is found, file name and path are shown on the console.

For testing purposes, an image called testbild.jpg is provided, containing the suspective S/N. It is small, red and shows the text "test" on it.

For the convenience of non-programming people, I have added binaries for Windows, Linux x86 and Linux AMD64. Please mind to add the cygwin1.dll to the dirr.exe file on Windows.

Please be aware of the facts that windows and unix/linux sources differ in some aspects due to the non-availability of certain GNU-routines on windows, e.g. memmem() byte sequence routine does not exist on Windows. The availability of GCC through cygwin does not mean that glibc is available on Windows.

Image processing software is generally preserving the S/N, so there might be images around with the S/N.

If you should ever detect images by use of this programme, please, PLEASE inform the German BKA about it. Police authorities need every hint to find the perpetrator.

If anyone can test and/or correct the program, please do so. A graphical interface might be reaching more people and would be welcome, too.
