#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "printRoutines.h"

#define ERROR_RETURN -1
#define SUCCESS 0

FILE *machineCode;

int main(int argc, char **argv) {

  FILE *outputFile;
  long currAddr = 0;

  // Verify that the command line has an appropriate number
  // of arguments

  if (argc < 2 || argc > 4) {
    fprintf(stderr, "Usage: %s InputFilename [OutputFilename] [startingOffset]\n", argv[0]);
    return ERROR_RETURN;
  }

  // First argument is the file to read, attempt to open it
  // for reading and verify that the open did occur.
  machineCode = fopen(argv[1], "rb");

  if (machineCode == NULL) {
    fprintf(stderr, "Failed to open %s: %s\n", argv[1], strerror(errno));
    return ERROR_RETURN;
  }

  // Second argument is the file to write, attempt to open it for
  // writing and verify that the open did occur. Use standard output
  // if not provided.
  outputFile = argc <= 2 ? stdout : fopen(argv[2], "w");

  if (outputFile == NULL) {
    fprintf(stderr, "Failed to open %s: %s\n", argv[2], strerror(errno));
    fclose(machineCode);
    return ERROR_RETURN;
  }

  // If there is a 3rd argument present it is an offset so convert it
  // to a numeric value.
  if (4 == argc) {
    errno = 0;
    currAddr = strtol(argv[3], NULL, 0);
    if (errno != 0) {
      perror("Invalid offset on command line");
      fclose(machineCode);
      fclose(outputFile);
      return ERROR_RETURN;
    }
  }

  fprintf(stderr, "Opened %s, starting offset 0x%lX\n", argv[1], currAddr);
  fprintf(stderr, "Saving output to %s\n", argc <= 2 ? "standard output" : argv[2]);

  // Comment or delete the following lines and this comment before
  // handing in your final version.
  // if (currAddr) printPosition(outputFile, currAddr);
  // printInstruction(outputFile);

  // Your code starts here.

  // some variables, variable names are self explanatory
  int fileSize;

  // go to end of machineCode & find file length
  fseek(machineCode, 0 , SEEK_END);
  fileSize = ftell(machineCode);
  rewind(machineCode);

  // Read the file into buffer
  char *buffer = (char*) malloc((fileSize + 1) * sizeof(char));
  fread(buffer, fileSize, 1, machineCode);

  // Buffer now contains all the data, convert
  converter(outputFile, buffer, fileSize, currAddr);

  free(buffer);
  fclose(machineCode);
  fclose(outputFile);
  return SUCCESS;
}

// Converter that calls appropriate functions
void converter(FILE* outputFile, char* buffer, int size, long currAddr){
  char haltPrint = 0;
  char emptyRegion = 0;
  char firstPos = 0;

  unsigned char firstByte = buffer[currAddr];
  unsigned char firstByte1 = (firstByte >> 4) & 15;
  unsigned char firstByte2 = firstByte & 15;

  if ((firstByte1 == 0) & (firstByte2 == 0)) firstPos = 1;
  if (currAddr > 0) firstPos = 1;

  while (currAddr < size){
    unsigned char data = buffer[currAddr];
    unsigned char firstHalf = (data >> 4) & 15;

    // Check for validity, if invalid, deal accordingly
    char * validity = checkValid(buffer, currAddr, size);
    if (strcmp(validity, "valid") != 0){
      if (((currAddr % 8) == 0) || ((size - currAddr) < 8)) {
        quadHandler(outputFile, buffer, currAddr);
        currAddr += 8;
        haltPrint = 1;
      }
      else{
        byteHandler(outputFile, buffer, currAddr);
        currAddr += 2;
        haltPrint = 1;
      }
      continue;
    }

    // start with pos if conditions are met
    if (firstPos & (firstHalf != 0)) {
      printPosition(outputFile, currAddr);
      firstPos = 0;
    }

    if ((firstHalf != 0x0) & (emptyRegion != 0)){
      fprintf(outputFile, "\n");
      printPosition(outputFile, currAddr);
      emptyRegion = 0;
    }

    // Dispatcher: goes to appropriate handler for each instruction
    switch(firstHalf) {
      case 0x0:
        if (haltPrint){
          oneByteHandler(outputFile, buffer, currAddr);
          haltPrint = 0;
          emptyRegion = 1;
        }
        currAddr +=1;
        continue;
      case 0x1:
        oneByteHandler(outputFile, buffer, currAddr);
        currAddr +=1;
        haltPrint = 1;
        continue;
      case 0x2:
        cmovHandler(outputFile, buffer, currAddr);
        currAddr +=2;
        haltPrint = 1;
        continue;
      case 0x3:
        irmovqHandler(outputFile, buffer, currAddr);
        currAddr +=10;
        haltPrint = 1;
        continue;
      case 0x4:
        rmmovqHandler(outputFile, buffer, currAddr);
        currAddr +=10;
        haltPrint = 1;
        continue;
      case 0x5:
        mrmovqHandler(outputFile, buffer, currAddr);
        currAddr +=10;
        haltPrint = 1;
        continue;
      case 0x6:
        OPqHandler(outputFile, buffer, currAddr);
        currAddr +=2;
        haltPrint = 1;
        continue;
      case 0x7:
        jXXHandler(outputFile, buffer, currAddr);
        currAddr +=9;
        haltPrint = 1;
        continue;
      case 0x8:
        callHandler(outputFile, buffer, currAddr);
        currAddr +=9;
        haltPrint = 1;
        continue;
      case 0x9:
        oneByteHandler(outputFile, buffer, currAddr);
        currAddr +=1;
        haltPrint = 1;
        continue;
      case 0xA:
        pushPopHandler(outputFile, buffer, currAddr);
        currAddr +=2;
        haltPrint = 1;
        continue;
      case 0xB:
        pushPopHandler(outputFile, buffer, currAddr);
        currAddr +=2;
        haltPrint = 1;
        continue;
      default: {
        printf("Invalid Instruction Encountered at pos %li\n", currAddr);
        break;
      }
    }
  }
}


void oneByteHandler (FILE* outputFile, char* buffer, long currAddr){
  unsigned char data = buffer[currAddr];
  unsigned char firstHalf = (data >> 4) & 15;
  unsigned char secondHalf = data & 15;
  char* instr = oneByteFn(firstHalf);
  fprintf(outputFile, "    %-8s                    # %x%x\n",
		 instr, firstHalf, secondHalf);
}

void cmovHandler (FILE* outputFile, char* buffer, long currAddr){
  unsigned char data = buffer[currAddr];
  unsigned char firstHalf = (data >> 4) & 15;
  unsigned char secondHalf = data & 15;
  char* instr = checkCmovFn(secondHalf);

  unsigned char reg = buffer[currAddr + 1];
  unsigned char regA = (reg >> 4) & 15;
  unsigned char regB = reg & 15;
  char* rA = checkRegister(regA, 0);
  char* rB = checkRegister(regB, 0);

  fprintf(outputFile, "    %-8s%s, %s          # %x%x%x%x\n",
		 instr, rA, rB, firstHalf, secondHalf, regA, regB);
}

void irmovqHandler (FILE* outputFile, char* buffer, long currAddr){
  unsigned char data = buffer[currAddr];
  unsigned char firstHalf = (data >> 4) & 15;
  unsigned char secondHalf = data & 15;
  char* instr = "irmovq";

  unsigned char reg = buffer[currAddr + 1];
  unsigned char regA = (reg >> 4) & 15;
  unsigned char regB = reg & 15;
  // char* rA = checkRegister(regA, 0);
  char* rB = checkRegister(regB, 0);

  // print instruction
  fprintf(outputFile, "    %-8s0x", instr);
  // print value
  printLittle(outputFile, buffer, currAddr + 2);
  // print register
  fprintf(outputFile, ", %s          ", rB);
  // print first half comment
  fprintf(outputFile, "# %x%x%x%x", firstHalf, secondHalf, regA, regB);
  // print second half comment
  printBig(outputFile, buffer, currAddr + 2);
  fprintf(outputFile, "\n");
}

void rmmovqHandler (FILE* outputFile, char* buffer, long currAddr){
  unsigned char data = buffer[currAddr];
  unsigned char firstHalf = (data >> 4) & 15;
  unsigned char secondHalf = data & 15;
  char* instr = "rmmovq";

  unsigned char reg = buffer[currAddr + 1];
  unsigned char regA = (reg >> 4)&15;
  unsigned char regB = reg & 15;
  char* rA = checkRegister(regA, 0);
  char* rB = checkRegister(regB, 0);

  // print instruction
  fprintf(outputFile, "    %-8s", instr);
  // print rA
  fprintf(outputFile, "%s, 0x", rA);
  // print value
  printLittle(outputFile, buffer, currAddr + 2);
  // print rB
  fprintf(outputFile, "(%s)", rB);
  // print first half comment
  fprintf(outputFile, "          # %x%x%x%x", firstHalf, secondHalf, regA, regB);
  // print second half comment
  printBig(outputFile, buffer, currAddr + 2);
  fprintf(outputFile, "\n");
}

void mrmovqHandler (FILE* outputFile, char* buffer, long currAddr){
  unsigned char data = buffer[currAddr];
  unsigned char firstHalf = (data >> 4) & 15;
  unsigned char secondHalf = data & 15;
  char* instr = "mrmovq";

  unsigned char reg = buffer[currAddr + 1];
  unsigned char regA = (reg >> 4) & 15;
  unsigned char regB = reg & 15;
  char* rA = checkRegister(regA, 0);
  char* rB = checkRegister(regB, 0);

  // print instruction
  fprintf(outputFile, "    %-8s0x(", instr);
  // print value
  printLittle(outputFile, buffer, currAddr + 2);
  // print register
  fprintf(outputFile, ")%s, %s 0x", rA, rB);
  // print first half comment
  fprintf(outputFile, "          # %x%x%x%x", firstHalf, secondHalf, regA, regB);
  // print second half comment
  printBig(outputFile, buffer, currAddr + 2);
  fprintf(outputFile, "\n");
}

void OPqHandler (FILE* outputFile, char* buffer, long currAddr){
  unsigned char data = buffer[currAddr];
  unsigned char firstHalf = data >> 4;
  unsigned char secondHalf = data & 15;
  char* instr = checkOpFn(secondHalf);

  unsigned char reg = buffer[currAddr + 1];
  unsigned char regA = reg >> 4;
  unsigned char regB = reg & 15;
  char* rA = checkRegister(regA, 0);
  char* rB = checkRegister(regB, 0);

  // print instruction
  fprintf(outputFile, "    %-8s", instr);
  // print register
  fprintf(outputFile, "%s, %s    ", rA, rB);
  // print comment
  fprintf(outputFile, "# %x%x%x%x\n", firstHalf, secondHalf, regA, regB);
}

void jXXHandler (FILE* outputFile, char* buffer, long currAddr){
  unsigned char data = buffer[currAddr];
  unsigned char firstHalf = data >> 4;
  unsigned char secondHalf = data & 15;
  char* instr = checkJmpFn(secondHalf);

  // print instruction
  fprintf(outputFile, "    %-8s0x", instr);
  // print value
  printLittle(outputFile, buffer, currAddr + 1);
  // print first half comment
  fprintf(outputFile, "         # %x%x", firstHalf, secondHalf);
  // print second half comment
  printBig(outputFile, buffer, currAddr + 1);
  fprintf(outputFile, "\n");
}

void callHandler (FILE* outputFile, char* buffer, long currAddr){
  unsigned char data = buffer[currAddr];
  unsigned char firstHalf = data >> 4;
  unsigned char secondHalf = data & 15;

  char* instr = "call";
  // print instruction
  fprintf(outputFile, "    %-8s0x", instr);
  // print value
  printLittle(outputFile, buffer, currAddr + 1);
  // print first half comment
  fprintf(outputFile, "         # %x%x", firstHalf, secondHalf);
  // print second half comment
  printBig(outputFile, buffer, currAddr + 1);
  fprintf(outputFile, "\n");
}

void pushPopHandler (FILE* outputFile, char* buffer, long currAddr){
  unsigned char data = buffer[currAddr];
  unsigned char firstHalf = (data >> 4) & 15;
  unsigned char secondHalf = data & 15;

  unsigned char reg = buffer[currAddr + 1];
  unsigned char regA = (reg >> 4) & 15;
  unsigned char regB = reg & 15;
  char* rA = checkRegister(regA, 0);

  char* instr = pushPopFn(firstHalf);

  fprintf(outputFile, "    %-8s%s                    # %x%x%x%x\n",
		 instr, rA, firstHalf, secondHalf, regA, regB);
}

void quadHandler (FILE* outputFile, char* buffer, long currAddr){
  // print instruction
  fprintf(outputFile, "    %-8s", ".quad");
  // print value
  printLittle(outputFile, buffer, currAddr);

  // print first half comment
  fprintf(outputFile, "             # ");
  // print second half comment
  printBig(outputFile, buffer, currAddr);
  fprintf(outputFile, "\n");
}

void byteHandler (FILE* outputFile, char* buffer, long currAddr){
  unsigned char data = buffer[currAddr];
  unsigned char firstHalf = (data >> 4) & 15;
  unsigned char secondHalf = data & 15;

  // print instruction
  fprintf(outputFile, "    %-8s", ".byte");
  // print value
  printLittle(outputFile, buffer, currAddr);

  // print comment
  fprintf(outputFile, "%x%x           # %x%x",
    firstHalf, secondHalf, firstHalf, secondHalf);
}

// Checks if instruction is valid. If valid, returns 0, else returns length
char * checkValid (char* buffer, long currAddr, int size){
  unsigned char data = buffer[currAddr];
  unsigned char firstHalf = (data >> 4) & 15;
  unsigned char secondHalf = data & 15;

  unsigned char reg = buffer[currAddr + 1];
  unsigned char regA = (reg >> 4)&15;
  unsigned char regB = reg & 15;

  switch(firstHalf) {
    case 0x0:
      if (secondHalf != 0) return "invalid";
      if ((size - currAddr) < 1) return "invalid";
      break;
    case 0x1:
      if (secondHalf != 0) return "invalid";
      if ((size - currAddr) < 1) return "invalid";
      break;
    case 0x2:
      if (strcmp(checkCmovFn(secondHalf), "invalid") == 0)
        return "invalid";
      if ((strcmp(checkRegister(regA, 0), "invalid") == 0) \
          || (strcmp(checkRegister(regB, 0), "invalid") == 0))
        return "invalid";
      if ((size - currAddr) < 2) return "invalid";
      break;
    case 0x3:
      if (secondHalf != 0){
        return "invalid";
      }
      if ((strcmp(checkRegister(regA, 1), "invalid") == 0) \
          || (strcmp(checkRegister(regB, 0), "invalid") == 0)){
        return "invalid";
      }
      if ((size - currAddr) < 10) return "invalid";
      break;
    case 0x4:
      if (secondHalf != 0) return "invalid";
      if ((strcmp(checkRegister(regA, 0), "invalid") == 0) \
          || (strcmp(checkRegister(regB, 0), "invalid") == 0))
        return "invalid";
      if ((size - currAddr) < 10) return "invalid";
      break;
    case 0x5:
      if (secondHalf != 0) return "invalid";
      if ((strcmp(checkRegister(regA, 0), "invalid") == 0) \
          || (strcmp(checkRegister(regB, 0), "invalid") == 0))
        return "invalid";
      if ((size - currAddr) < 10) return "invalid";
      break;
    case 0x6:
      if ((secondHalf != 0) && (strcmp(checkOpFn(secondHalf), "invalid")) == 0)
        return "invalid";
      if ((strcmp(checkRegister(regA, 0), "invalid") == 0) \
          || (strcmp(checkRegister(regB, 0), "invalid") == 0))
        return "invalid";
      if ((size - currAddr) < 2) return "invalid";
      break;
    case 0x7:
      if ((secondHalf != 0) && (strcmp(checkJmpFn(secondHalf), "invalid")) == 0)
        return "invalid";
      if ((size - currAddr) < 9) return "invalid";
      break;
    case 0x8:
      if (secondHalf != 0) return "invalid";
      if ((size - currAddr) < 9) return "invalid";
      break;
    case 0x9:
      if (secondHalf != 0) return "invalid";
      if ((size - currAddr) < 1) return "invalid";
      break;
    case 0xA:
      if (secondHalf != 0) return "invalid";
      if ((strcmp(checkRegister(regA, 0), "invalid") == 0) \
          || (strcmp(checkRegister(regB, 1), "invalid") == 0))
        return "invalid";
      if ((size - currAddr) < 2) return "invalid";
      break;
    case 0xB:
      if (secondHalf != 0) return "invalid";
      if ((strcmp(checkRegister(regA, 0), "invalid") == 0) \
          || (strcmp(checkRegister(regB, 1), "invalid") == 0))
        return "invalid";
      if ((size - currAddr) < 2) return "invalid";
      break;
    default: {
      return "invalid";
      break;
    }
  }
  return "valid";
}

// finds fn name for push pop
char * pushPopFn(char fn){
  switch (fn) {
    case 0xA:
      return "pushq";
      break;
    case 0xB:
      return "popq";
      break;
    default:
      return "invalid";
  }
}

// finds fn name for halt, nop, ret
char * oneByteFn(char fn){
  switch (fn) {
    case 0x0:
      return "halt";
      break;
    case 0x1:
      return "nop";
      break;
    case 0x9:
      return "ret";
      break;
    default:
      return "invalid";
  }
}

// Checks validity of Op functions
char * checkOpFn(char fn){
  switch (fn) {
    case 0x0:
      return "addq";
      break;
    case 0x1:
      return "subq";
      break;
    case 0x2:
      return "andq";
      break;
    case 0x3:
      return "xorq";
      break;
    case 0x4:
      return "mulq";
      break;
    case 0x5:
      return "divq";
      break;
    case 0x6:
      return "modq";
      break;
    default:
      return "invalid";
  }
}

// Checks validity of Jump functions
char * checkJmpFn(char fn){
  switch (fn) {
    case 0x0:
      return "jmp";
      break;
    case 0x1:
      return "jle";
      break;
    case 0x2:
      return "jl";
      break;
    case 0x3:
      return "je";
      break;
    case 0x4:
      return "jne";
      break;
    case 0x5:
      return "jge";
      break;
    case 0x6:
      return "jg";
      break;
    default:
      return "invalid";
  }
}

// Checks validity of Conditional Move Functions, including rrmov
char * checkCmovFn(char fn){
  switch (fn) {
    case 0x0:
      return "rrmovq";
      break;
    case 0x1:
      return "cmovle";
      break;
    case 0x2:
      return "cmovl";
      break;
    case 0x3:
      return "cmove";
      break;
    case 0x4:
      return "cmovne";
      break;
    case 0x5:
      return "cmovge";
      break;
    case 0x6:
      return "cmovg";
      break;
    default:
      return "invalid";
  }
}

// Checks validity of Registers
char * checkRegister(char reg, char isF){
  switch (reg) {
    case 0x0:
      return "%rax";
      break;
    case 0x1:
      return "%rcx";
      break;
    case 0x2:
      return "%rdx";
      break;
    case 0x3:
      return "%rbx";
      break;
    case 0x4:
      return "%rsp";
      break;
    case 0x5:
      return "%rbp";
      break;
    case 0x6:
      return "%rsi";
      break;
    case 0x7:
      return "%rdi";
      break;
    case 0x8:
      return "%r8";
      break;
    case 0x9:
      return "%r9";
      break;
    case 0xA:
      return "%r10";
      break;
    case 0xB:
      return "%r11";
      break;
    case 0xC:
      return "%r12";
      break;
    case 0xD:
      return "%r13";
      break;
    case 0xE:
      return "%r14";
      break;
    case 0xF:
      if(isF == 1) return "F";
      return "invalid";
    default:
      return "invalid";
  }
}

void printLittle(FILE * outputFile, char * buffer, long currAddr){
  unsigned char * val = malloc(sizeof(char) * 16);

  // store in an array
  for (int i = 0; i < 8; i++){
    val[(7-i)*2] = (buffer[currAddr + i] >> 4) & 15;
    val[(7-i)*2 + 1] = buffer[currAddr + i] & 15;
  }

  char canPrint = 0;
  char printed = 0;
  for (int i = 0; i< 16; i++){
    // Check if canPrint
    if (val[i] != 0) canPrint = 1;
    // Print once can print
    if (canPrint == 1){
      fprintf(outputFile, "%x", val[i]);
      printed = 1;
    }
    // Print 0 if last member of array, and havn't printed at all
    if ((i == 15) & (printed == 0)) {
      fprintf(outputFile, "%x", val[i]);
    }
  }
}

void printBig(FILE * outputFile, char * buffer, long currAddr){
  unsigned char * val = malloc(sizeof(char) * 16);

  // store in an array
  for (int i = 0; i < 8; i++){
    val[i*2] = (buffer[currAddr + i] >> 4) & 15;
    val[i*2 + 1] = buffer[currAddr + i] & 15;
  }

  for (int i = 0; i< 16; i++){
    fprintf(outputFile, "%x", val[i]);
  }
}
