#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "printRoutines.h"



int printPosition(FILE *out, unsigned long location) {

  return fprintf(out, ".pos 0x%lx\n", location);
}

int printInstruction(FILE *out) {

  int res = 0;

  char * r1 = "%rax";
  char * r2 = "%rdx";
  char * inst1 = "rrmovq";
  char * inst2 = "jne";
  char * inst3 = "irmovq";
  char * inst4 = "mrmovq";
  unsigned long destAddr = 8193;

  res += fprintf(out, "    %-8s%s, %s          # %-22s\n",
		 inst1, r1, r2, "2002");

  res += fprintf(out, "    %-8s0x%lx              # %-22s\n",
		 inst2, destAddr, "740120000000000000");

  res += fprintf(out, "    %-8s$0x%lx, %s         # %-22s\n",
		 inst3, 16L, r2, "30F21000000000000000");

  res += fprintf(out, "    %-8s0x%lx(%s), %s # %-22s\n",
		 inst4, 65536L, r2, r1, "50020000010000000000");

  res += fprintf(out, "    %-8s%s, %s          # %-22s\n",
		 inst1, r2, r1, "2020");

  res += fprintf(out, "    %-8s0x%lx  # %-22s\n",
		 ".quad", 0xFFFFFFFFFFFFFFFFL, "FFFFFFFFFFFFFFFF");

  return res;
}
