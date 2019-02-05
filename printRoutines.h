/* This file contains the prototypes and constants needed to use the
   routines defined in printRoutines.c
*/

#ifndef _PRINTROUTINES_H_
#define _PRINTROUTINES_H_

#include <stdio.h>

// functions from printRoutines
int printPosition(FILE *, unsigned long);
// void printIns(FILE *, char*, char*, char*, char*, char*);
// int printInstruction(FILE *);

// functions from disassembler
void converter(FILE*, char*, int, long);
char * checkValid (char* , long, int);
char * checkRegister(char, char);
char * checkOpFn(char);
char * checkJmpFn(char);
char * checkCmovFn(char);
char * oneByteFn(char);
char * pushPopFn(char);

void oneByteHandler (FILE*, char*, long);
void cmovHandler (FILE*, char*, long);
void irmovqHandler (FILE*, char*, long);
void rmmovqHandler (FILE*, char*, long);
void mrmovqHandler (FILE*, char*, long);
void OPqHandler (FILE*, char*, long);
void jXXHandler (FILE*, char*, long);
void callHandler (FILE*, char*, long);
void pushPopHandler (FILE*, char*, long);
void quadHandler (FILE*, char*, long);
void byteHandler (FILE*, char*, long);

void printLittle(FILE *, char *, long);
void printBig(FILE *, char *, long);
// unsigned char * getSome(int, long);



#endif /* PRINTROUTINES */
