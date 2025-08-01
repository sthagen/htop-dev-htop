#ifndef HEADER_Platform
#define HEADER_Platform
/*
htop - solaris/Platform.h
(C) 2014 Hisham H. Muhammad
(C) 2015 David C. Hunt
(C) 2017,2018 Guy M. Broome
Released under the GNU GPLv2+, see the COPYING file
in the source distribution for its full text.
*/

#include <kstat.h>

/* On OmniOS /usr/include/sys/regset.h redefines ERR to 13 - \r, breaking the Enter key.
 * Since ncruses macros use the ERR macro, we can not use another name.
 */
#undef ERR
#include <libproc.h>
#undef ERR
#define ERR (-1)

#include <signal.h>
#include <stdbool.h>

#include <sys/mkdev.h>
#include <sys/proc.h>
#include <sys/types.h>

#include "Action.h"
#include "BatteryMeter.h"
#include "CommandLine.h"
#include "DiskIOMeter.h"
#include "Hashtable.h"
#include "NetworkIOMeter.h"
#include "ProcessLocksScreen.h"
#include "SignalsPanel.h"
#include "generic/gettime.h"
#include "generic/hostname.h"
#include "generic/uname.h"


#define  kill(pid, signal) kill(pid / 1024, signal)

typedef struct var kvar_t;

typedef struct envAccum_ {
   size_t capacity;
   size_t size;
   size_t bytes;
   char* env;
} envAccum;

extern const ScreenDefaults Platform_defaultScreens[];

extern const unsigned int Platform_numberOfDefaultScreens;

extern const SignalItem Platform_signals[];

extern const unsigned int Platform_numberOfSignals;

extern const MeterClass* const Platform_meterTypes[];

bool Platform_init(void);

void Platform_done(void);

void Platform_setBindings(Htop_Action* keys);

int Platform_getUptime(void);

void Platform_getLoadAverage(double* one, double* five, double* fifteen);

pid_t Platform_getMaxPid(void);

double Platform_setCPUValues(Meter* this, unsigned int cpu);

void Platform_setMemoryValues(Meter* this);

void Platform_setSwapValues(Meter* this);

void Platform_setZfsArcValues(Meter* this);

void Platform_setZfsCompressedArcValues(Meter* this);

char* Platform_getProcessEnv(pid_t pid);

FileLocks_ProcessData* Platform_getProcessLocks(pid_t pid);

void Platform_getFileDescriptors(double* used, double* max);

bool Platform_getDiskIO(DiskIOData* data);

bool Platform_getNetworkIO(NetworkIOData* data);

void Platform_getBattery(double* percent, ACPresence* isOnAC);

static inline void Platform_getHostname(char* buffer, size_t size) {
   Generic_hostname(buffer, size);
}

static inline void Platform_getRelease(char** string) {
   *string = Generic_uname();
}

static inline const char* Platform_getFailedState(void) {
   return NULL;
}

#define PLATFORM_LONG_OPTIONS

static inline void Platform_longOptionsUsage(ATTR_UNUSED const char* name) { }

static inline CommandLineStatus Platform_getLongOption(ATTR_UNUSED int opt, ATTR_UNUSED int argc, ATTR_UNUSED char** argv) {
   return STATUS_ERROR_EXIT;
}

static inline void Platform_gettime_realtime(struct timeval* tv, uint64_t* msec) {
   Generic_gettime_realtime(tv, msec);
}

static inline void Platform_gettime_monotonic(uint64_t* msec) {
   Generic_gettime_monotonic(msec);
}

static inline void* kstat_data_lookup_wrapper(kstat_t* ksp, const char* name) {
IGNORE_WCASTQUAL_BEGIN
   return kstat_data_lookup(ksp, (char*)name);
IGNORE_WCASTQUAL_END
}

static inline kstat_t* kstat_lookup_wrapper(kstat_ctl_t* kc, const char* ks_module, int ks_instance, const char* ks_name) {
IGNORE_WCASTQUAL_BEGIN
   return kstat_lookup(kc, (char*)ks_module, ks_instance, (char*)ks_name);
IGNORE_WCASTQUAL_END
}

static inline Hashtable* Platform_dynamicMeters(void) {
   return NULL;
}

static inline void Platform_dynamicMetersDone(ATTR_UNUSED Hashtable* table) { }

static inline void Platform_dynamicMeterInit(ATTR_UNUSED Meter* meter) { }

static inline void Platform_dynamicMeterUpdateValues(ATTR_UNUSED Meter* meter) { }

static inline void Platform_dynamicMeterDisplay(ATTR_UNUSED const Meter* meter, ATTR_UNUSED RichString* out) { }

static inline Hashtable* Platform_dynamicColumns(void) {
   return NULL;
}

static inline void Platform_dynamicColumnsDone(ATTR_UNUSED Hashtable* table) { }

static inline const char* Platform_dynamicColumnName(ATTR_UNUSED unsigned int key) {
   return NULL;
}

static inline bool Platform_dynamicColumnWriteField(ATTR_UNUSED const Process* proc, ATTR_UNUSED RichString* str, ATTR_UNUSED unsigned int key) {
   return false;
}

static inline Hashtable* Platform_dynamicScreens(void) {
   return NULL;
}

static inline void Platform_defaultDynamicScreens(ATTR_UNUSED Settings* settings) { }

static inline void Platform_addDynamicScreen(ATTR_UNUSED ScreenSettings* ss) { }

static inline void Platform_addDynamicScreenAvailableColumns(ATTR_UNUSED Panel* availableColumns, ATTR_UNUSED const char* screen) { }

static inline void Platform_dynamicScreensDone(ATTR_UNUSED Hashtable* screens) { }

#endif
