#ifndef HEADER_DynamicMeter
#define HEADER_DynamicMeter
/*
htop - DynamicMeter.h
(C) 2021 htop dev team
(C) 2021 Red Hat, Inc.  All Rights Reserved.
Released under the GNU GPLv2+, see the COPYING file
in the source distribution for its full text.
*/

#include <stdbool.h>

#include "Hashtable.h"
#include "Meter.h"


typedef struct DynamicMeter_ {
   char name[32];  /* unique name, cannot contain spaces */
   char* caption;
   char* description;
   unsigned int type;
   double maximum;
} DynamicMeter;

Hashtable* DynamicMeters_new(void);

void DynamicMeters_delete(Hashtable* dynamics);

const char* DynamicMeter_lookup(Hashtable* dynamics, ht_key_t key);

bool DynamicMeter_search(Hashtable* dynamics, const char* name, ht_key_t* key);

extern const MeterClass DynamicMeter_class;

#endif
