// boards.h - defines for the different ESP32 boards

//----------------------------------------------------
// file: boards.h
// description: board definitions for Nova
// language: C++
// Target processor: ESP32-S3-MINI-N4R2
// Author: GlennCB (glenncb@p3pd.com)
// Created: Wednesday, February 14, 2024 6:55:14 AM
// Copyright: (C) 2024, Phase Three Product Development
//-----------------------------------------------------

#pragma once
//#include "globals.h"
#include <stdint.h>

#define BOARDNAME_MAXCHARS 16

// drop delay board
typedef enum {
  NOBOARD     = 0,    // master ID, not to be used
  POWERCTL,
  MANIFOLD,
  PANELDISP,
  FLUIDCART,
  DROPDELAY,
  VENTSAFE,
  INCUBATOR,
  HIGHVOLT,
} board_t;  

typedef struct {
  board_t id;
  char rev;
  const char *name;
} boardinfotab_t;

// board information
typedef struct __attribute__((packed)) {
  uint8_t boardrev;
  uint8_t boardid;
} boardinfo_t;

// firmware version
typedef struct __attribute__((packed)) {
  uint8_t fwrev_minor;
  uint8_t fwrev_major;
} fwinfo_t;

extern const boardinfotab_t boardtab[];
//extern const boardinfo_t boardinfo;
//extern const fwinfo_t fwinfo;
