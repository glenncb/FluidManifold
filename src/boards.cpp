// boards.cpp - board specific globals and routines
#include "boards.h"

// Board information
const boardinfotab_t boardtab[] = {
  {.id=NOBOARD,   .rev='A',   .name="NOBOARD"},
  {.id=POWERCTL,  .rev='A',   .name="PWRCTL"},
  {.id=MANIFOLD,  .rev='A',   .name="MANIFOLD"},
  {.id=PANELDISP, .rev='A',   .name="DISPLAY"},
  {.id=FLUIDCART, .rev='B',   .name="FLUIDCART"},
  {.id=DROPDELAY, .rev='C',   .name="DROPDELAY"},
  {.id=VENTSAFE,  .rev='A',   .name="VENTSAFE"},
  {.id=INCUBATOR, .rev='C',   .name="INCUBATOR"},
  {.id=HIGHVOLT,  .rev='A',   .name="HIGHVOLT"}
};
