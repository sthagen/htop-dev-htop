/*
htop - linux/Platform.c
(C) 2014 Hisham H. Muhammad
(C) 2020-2022 htop dev team
(C) 2020-2022 Red Hat, Inc.
Released under the GNU GPLv2+, see the COPYING file
in the source distribution for its full text.
*/

#include "config.h" // IWYU pragma: keep

#include "pcp/Platform.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "BatteryMeter.h"
#include "CPUMeter.h"
#include "ClockMeter.h"
#include "DateMeter.h"
#include "DateTimeMeter.h"
#include "DiskIOMeter.h"
#include "DynamicColumn.h"
#include "DynamicMeter.h"
#include "DynamicScreen.h"
#include "FileDescriptorMeter.h"
#include "HostnameMeter.h"
#include "LoadAverageMeter.h"
#include "Macros.h"
#include "MemoryMeter.h"
#include "MemorySwapMeter.h"
#include "Meter.h"
#include "NetworkIOMeter.h"
#include "ProcessTable.h"
#include "Settings.h"
#include "SwapMeter.h"
#include "SysArchMeter.h"
#include "TasksMeter.h"
#include "UptimeMeter.h"
#include "XUtils.h"

#include "linux/PressureStallMeter.h"
#include "linux/ZramMeter.h"
#include "linux/ZramStats.h"
#include "pcp/Metric.h"
#include "pcp/PCPDynamicColumn.h"
#include "pcp/PCPDynamicMeter.h"
#include "pcp/PCPDynamicScreen.h"
#include "pcp/PCPMachine.h"
#include "pcp/PCPProcessTable.h"
#include "zfs/ZfsArcMeter.h"
#include "zfs/ZfsArcStats.h"
#include "zfs/ZfsCompressedArcMeter.h"


Platform* pcp;

const ScreenDefaults Platform_defaultScreens[] = {
   {
      .name = "Main",
      .columns = "PID USER PRIORITY NICE M_VIRT M_RESIDENT M_SHARE STATE PERCENT_CPU PERCENT_MEM TIME Command",
      .sortKey = "PERCENT_CPU",
   },
   {
      .name = "I/O",
      .columns = "PID USER IO_PRIORITY IO_RATE IO_READ_RATE IO_WRITE_RATE PERCENT_SWAP_DELAY PERCENT_IO_DELAY Command",
      .sortKey = "IO_RATE",
   },
};

const unsigned int Platform_numberOfDefaultScreens = ARRAYSIZE(Platform_defaultScreens);

const SignalItem Platform_signals[] = {
   { .name = " 0 Cancel",    .number = 0 },
};

const unsigned int Platform_numberOfSignals = ARRAYSIZE(Platform_signals);

const MeterClass* const Platform_meterTypes[] = {
   &CPUMeter_class,
   &ClockMeter_class,
   &DateMeter_class,
   &DateTimeMeter_class,
   &LoadAverageMeter_class,
   &LoadMeter_class,
   &MemoryMeter_class,
   &SwapMeter_class,
   &MemorySwapMeter_class,
   &TasksMeter_class,
   &UptimeMeter_class,
   &BatteryMeter_class,
   &HostnameMeter_class,
   &AllCPUsMeter_class,
   &AllCPUs2Meter_class,
   &AllCPUs4Meter_class,
   &AllCPUs8Meter_class,
   &LeftCPUsMeter_class,
   &RightCPUsMeter_class,
   &LeftCPUs2Meter_class,
   &RightCPUs2Meter_class,
   &LeftCPUs4Meter_class,
   &RightCPUs4Meter_class,
   &LeftCPUs8Meter_class,
   &RightCPUs8Meter_class,
   &PressureStallCPUSomeMeter_class,
   &PressureStallIOSomeMeter_class,
   &PressureStallIOFullMeter_class,
   &PressureStallIRQFullMeter_class,
   &PressureStallMemorySomeMeter_class,
   &PressureStallMemoryFullMeter_class,
   &ZfsArcMeter_class,
   &ZfsCompressedArcMeter_class,
   &ZramMeter_class,
   &DiskIOMeter_class,
   &NetworkIOMeter_class,
   &SysArchMeter_class,
   &FileDescriptorMeter_class,
   &BlankMeter_class,
   &DynamicMeter_class,
   NULL
};

static const char* Platform_metricNames[] = {
   [PCP_CONTROL_THREADS] = "proc.control.perclient.threads",

   [PCP_HINV_NCPU] = "hinv.ncpu",
   [PCP_HINV_NDISK] = "hinv.ndisk",
   [PCP_HINV_CPUCLOCK] = "hinv.cpu.clock",
   [PCP_UNAME_SYSNAME] = "kernel.uname.sysname",
   [PCP_UNAME_RELEASE] = "kernel.uname.release",
   [PCP_UNAME_MACHINE] = "kernel.uname.machine",
   [PCP_UNAME_DISTRO] = "kernel.uname.distro",
   [PCP_LOAD_AVERAGE] = "kernel.all.load",
   [PCP_PID_MAX] = "kernel.all.pid_max",
   [PCP_UPTIME] = "kernel.all.uptime",
   [PCP_BOOTTIME] = "kernel.all.boottime",
   [PCP_CPU_USER] = "kernel.all.cpu.user",
   [PCP_CPU_NICE] = "kernel.all.cpu.nice",
   [PCP_CPU_SYSTEM] = "kernel.all.cpu.sys",
   [PCP_CPU_IDLE] = "kernel.all.cpu.idle",
   [PCP_CPU_IOWAIT] = "kernel.all.cpu.wait.total",
   [PCP_CPU_IRQ] = "kernel.all.cpu.intr",
   [PCP_CPU_SOFTIRQ] = "kernel.all.cpu.irq.soft",
   [PCP_CPU_STEAL] = "kernel.all.cpu.steal",
   [PCP_CPU_GUEST] = "kernel.all.cpu.guest",
   [PCP_CPU_GUESTNICE] = "kernel.all.cpu.guest_nice",
   [PCP_PERCPU_USER] = "kernel.percpu.cpu.user",
   [PCP_PERCPU_NICE] = "kernel.percpu.cpu.nice",
   [PCP_PERCPU_SYSTEM] = "kernel.percpu.cpu.sys",
   [PCP_PERCPU_IDLE] = "kernel.percpu.cpu.idle",
   [PCP_PERCPU_IOWAIT] = "kernel.percpu.cpu.wait.total",
   [PCP_PERCPU_IRQ] = "kernel.percpu.cpu.intr",
   [PCP_PERCPU_SOFTIRQ] = "kernel.percpu.cpu.irq.soft",
   [PCP_PERCPU_STEAL] = "kernel.percpu.cpu.steal",
   [PCP_PERCPU_GUEST] = "kernel.percpu.cpu.guest",
   [PCP_PERCPU_GUESTNICE] = "kernel.percpu.cpu.guest_nice",
   [PCP_MEM_TOTAL] = "mem.physmem",
   [PCP_MEM_FREE] = "mem.util.free",
   [PCP_MEM_AVAILABLE] = "mem.util.available",
   [PCP_MEM_BUFFERS] = "mem.util.bufmem",
   [PCP_MEM_CACHED] = "mem.util.cached",
   [PCP_MEM_SHARED] = "mem.util.shmem",
   [PCP_MEM_SRECLAIM] = "mem.util.slabReclaimable",
   [PCP_MEM_SWAPCACHED] = "mem.util.swapCached",
   [PCP_MEM_SWAPTOTAL] = "mem.util.swapTotal",
   [PCP_MEM_SWAPFREE] = "mem.util.swapFree",
   [PCP_DISK_READB] = "disk.all.read_bytes",
   [PCP_DISK_WRITEB] = "disk.all.write_bytes",
   [PCP_DISK_ACTIVE] = "disk.all.avactive",
   [PCP_NET_RECVB] = "network.all.in.bytes",
   [PCP_NET_SENDB] = "network.all.out.bytes",
   [PCP_NET_RECVP] = "network.all.in.packets",
   [PCP_NET_SENDP] = "network.all.out.packets",

   [PCP_PSI_CPUSOME] = "kernel.all.pressure.cpu.some.avg",
   [PCP_PSI_IOSOME] = "kernel.all.pressure.io.some.avg",
   [PCP_PSI_IOFULL] = "kernel.all.pressure.io.full.avg",
   [PCP_PSI_IRQFULL] = "kernel.all.pressure.irq.full.avg",
   [PCP_PSI_MEMSOME] = "kernel.all.pressure.memory.some.avg",
   [PCP_PSI_MEMFULL] = "kernel.all.pressure.memory.full.avg",

   [PCP_ZFS_ARC_ANON_SIZE] = "zfs.arc.anon_size",
   [PCP_ZFS_ARC_BONUS_SIZE] = "zfs.arc.bonus_size",
   [PCP_ZFS_ARC_COMPRESSED_SIZE] = "zfs.arc.compressed_size",
   [PCP_ZFS_ARC_UNCOMPRESSED_SIZE] = "zfs.arc.uncompressed_size",
   [PCP_ZFS_ARC_C_MIN] = "zfs.arc.c_min",
   [PCP_ZFS_ARC_C_MAX] = "zfs.arc.c_max",
   [PCP_ZFS_ARC_DBUF_SIZE] = "zfs.arc.dbuf_size",
   [PCP_ZFS_ARC_DNODE_SIZE] = "zfs.arc.dnode_size",
   [PCP_ZFS_ARC_HDR_SIZE] = "zfs.arc.hdr_size",
   [PCP_ZFS_ARC_MFU_SIZE] = "zfs.arc.mfu.size",
   [PCP_ZFS_ARC_MRU_SIZE] = "zfs.arc.mru.size",
   [PCP_ZFS_ARC_SIZE] = "zfs.arc.size",

   [PCP_ZRAM_CAPACITY] = "zram.capacity",
   [PCP_ZRAM_ORIGINAL] = "zram.mm_stat.data_size.original",
   [PCP_ZRAM_COMPRESSED] = "zram.mm_stat.data_size.compressed",
   [PCP_MEM_ZSWAP] = "mem.util.zswap",
   [PCP_MEM_ZSWAPPED] = "mem.util.zswapped",
   [PCP_VFS_FILES_COUNT] = "vfs.files.count",
   [PCP_VFS_FILES_MAX] = "vfs.files.max",

   [PCP_PROC_PID] = "proc.psinfo.pid",
   [PCP_PROC_PPID] = "proc.psinfo.ppid",
   [PCP_PROC_TGID] = "proc.psinfo.tgid",
   [PCP_PROC_PGRP] = "proc.psinfo.pgrp",
   [PCP_PROC_SESSION] = "proc.psinfo.session",
   [PCP_PROC_STATE] = "proc.psinfo.sname",
   [PCP_PROC_TTY] = "proc.psinfo.tty",
   [PCP_PROC_TTYPGRP] = "proc.psinfo.tty_pgrp",
   [PCP_PROC_MINFLT] = "proc.psinfo.minflt",
   [PCP_PROC_MAJFLT] = "proc.psinfo.maj_flt",
   [PCP_PROC_CMINFLT] = "proc.psinfo.cmin_flt",
   [PCP_PROC_CMAJFLT] = "proc.psinfo.cmaj_flt",
   [PCP_PROC_UTIME] = "proc.psinfo.utime",
   [PCP_PROC_STIME] = "proc.psinfo.stime",
   [PCP_PROC_CUTIME] = "proc.psinfo.cutime",
   [PCP_PROC_CSTIME] = "proc.psinfo.cstime",
   [PCP_PROC_PRIORITY] = "proc.psinfo.priority",
   [PCP_PROC_NICE] = "proc.psinfo.nice",
   [PCP_PROC_THREADS] = "proc.psinfo.threads",
   [PCP_PROC_STARTTIME] = "proc.psinfo.start_time",
   [PCP_PROC_PROCESSOR] = "proc.psinfo.processor",
   [PCP_PROC_CMD] = "proc.psinfo.cmd",
   [PCP_PROC_PSARGS] = "proc.psinfo.psargs",
   [PCP_PROC_CGROUPS] = "proc.psinfo.cgroups",
   [PCP_PROC_OOMSCORE] = "proc.psinfo.oom_score",
   [PCP_PROC_VCTXSW] = "proc.psinfo.vctxsw",
   [PCP_PROC_NVCTXSW] = "proc.psinfo.nvctxsw",
   [PCP_PROC_LABELS] = "proc.psinfo.labels",
   [PCP_PROC_ENVIRON] = "proc.psinfo.environ",
   [PCP_PROC_TTYNAME] = "proc.psinfo.ttyname",
   [PCP_PROC_EXE] = "proc.psinfo.exe",
   [PCP_PROC_CWD] = "proc.psinfo.cwd",
   [PCP_PROC_AUTOGROUP_ID] = "proc.autogroup.id",
   [PCP_PROC_AUTOGROUP_NICE] = "proc.autogroup.nice",
   [PCP_PROC_ID_UID] = "proc.id.uid",
   [PCP_PROC_ID_USER] = "proc.id.uid_nm",
   [PCP_PROC_IO_RCHAR] = "proc.io.rchar",
   [PCP_PROC_IO_WCHAR] = "proc.io.wchar",
   [PCP_PROC_IO_SYSCR] = "proc.io.syscr",
   [PCP_PROC_IO_SYSCW] = "proc.io.syscw",
   [PCP_PROC_IO_READB] = "proc.io.read_bytes",
   [PCP_PROC_IO_WRITEB] = "proc.io.write_bytes",
   [PCP_PROC_IO_CANCELLED] = "proc.io.cancelled_write_bytes",
   [PCP_PROC_MEM_SIZE] = "proc.memory.size",
   [PCP_PROC_MEM_RSS] = "proc.memory.rss",
   [PCP_PROC_MEM_SHARE] = "proc.memory.share",
   [PCP_PROC_MEM_TEXTRS] = "proc.memory.textrss",
   [PCP_PROC_MEM_LIBRS] = "proc.memory.librss",
   [PCP_PROC_MEM_DATRS] = "proc.memory.datrss",
   [PCP_PROC_MEM_DIRTY] = "proc.memory.dirty",
   [PCP_PROC_SMAPS_PSS] = "proc.smaps.pss",
   [PCP_PROC_SMAPS_SWAP] = "proc.smaps.swap",
   [PCP_PROC_SMAPS_SWAPPSS] = "proc.smaps.swappss",

   [PCP_METRIC_COUNT] = NULL
};

#ifndef HAVE_PMLOOKUPDESCS
/*
 * pmLookupDescs(3) exists in latest versions of libpcp (5.3.6+),
 * but for older versions we provide an implementation here. This
 * involves multiple round trips to pmcd though, which the latest
 * libpcp version avoids by using a protocol extension.  In time,
 * perhaps in a few years, we could remove this back-compat code.
 */
int pmLookupDescs(int numpmid, pmID* pmids, pmDesc* descs) {
   int count = 0;

   for (int i = 0; i < numpmid; i++) {
      /* expect some metrics to be missing - e.g. PMDA not available */
      if (pmids[i] == PM_ID_NULL)
         continue;

      int sts = pmLookupDesc(pmids[i], &descs[i]);
      if (sts < 0) {
         if (pmDebugOptions.appl0)
            fprintf(stderr, "Error: cannot lookup metric %s(%s): %s\n",
                    pcp->names[i], pmIDStr(pcp->pmids[i]), pmErrStr(sts));
         pmids[i] = PM_ID_NULL;
         continue;
      }

      count++;
   }
   return count;
}
#endif

size_t Platform_addMetric(Metric id, const char* name) {
   unsigned int i = (unsigned int)id;

   if (i >= PCP_METRIC_COUNT && i >= pcp->totalMetrics) {
      /* added via configuration files */
      size_t j = pcp->totalMetrics + 1;
      pcp->fetch = xRealloc(pcp->fetch, j * sizeof(pmID));
      pcp->pmids = xRealloc(pcp->pmids, j * sizeof(pmID));
      pcp->names = xRealloc(pcp->names, j * sizeof(char*));
      pcp->descs = xRealloc(pcp->descs, j * sizeof(pmDesc));
      memset(&pcp->descs[i], 0, sizeof(pmDesc));
   }

   pcp->pmids[i] = pcp->fetch[i] = PM_ID_NULL;
   pcp->names[i] = name;
   return ++pcp->totalMetrics;
}

/* global state from the environment and command line arguments */
pmOptions opts;

bool Platform_init(void) {
   const char* source;
   if (opts.context == PM_CONTEXT_ARCHIVE) {
      source = opts.archives[0];
   } else if (opts.context == PM_CONTEXT_HOST) {
      source = opts.nhosts > 0 ? opts.hosts[0] : "local:";
   } else {
      opts.context = PM_CONTEXT_HOST;
      source = "local:";
   }

   int sts;
   sts = pmNewContext(opts.context, source);
   /* with no host requested, fallback to PM_CONTEXT_LOCAL shared libraries */
   if (sts < 0 && opts.context == PM_CONTEXT_HOST && opts.nhosts == 0) {
      opts.context = PM_CONTEXT_LOCAL;
      sts = pmNewContext(opts.context, NULL);
   }
   if (sts < 0) {
      fprintf(stderr, "Cannot setup PCP metric source: %s\n", pmErrStr(sts));
      return false;
   }
   /* setup timezones and other general startup preparation completion */
   if (pmGetContextOptions(sts, &opts) < 0 || opts.errors) {
      pmflush();
      return false;
   }

   pcp = xCalloc(1, sizeof(Platform));
   pcp->context = sts;
   pcp->fetch = xCalloc(PCP_METRIC_COUNT, sizeof(pmID));
   pcp->pmids = xCalloc(PCP_METRIC_COUNT, sizeof(pmID));
   pcp->names = xCalloc(PCP_METRIC_COUNT, sizeof(char*));
   pcp->descs = xCalloc(PCP_METRIC_COUNT, sizeof(pmDesc));

   if (opts.context == PM_CONTEXT_ARCHIVE) {
      gettimeofday(&pcp->offset, NULL);
#if PMAPI_VERSION >= 3
      struct timeval start = { opts.start.tv_sec, opts.start.tv_nsec / 1000 };
      pmtimevalDec(&pcp->offset, &start);
#else
      pmtimevalDec(&pcp->offset, &opts.start);
#endif
   }

   for (unsigned int i = 0; i < PCP_METRIC_COUNT; i++)
      Platform_addMetric(i, Platform_metricNames[i]);
   pcp->meters.offset = PCP_METRIC_COUNT;

   PCPDynamicMeters_init(&pcp->meters);

   pcp->columns.offset = PCP_METRIC_COUNT + pcp->meters.cursor;
   PCPDynamicColumns_init(&pcp->columns);
   PCPDynamicScreens_init(&pcp->screens, &pcp->columns);

   sts = pmLookupName(pcp->totalMetrics, pcp->names, pcp->pmids);
   if (sts < 0) {
      fprintf(stderr, "Error: cannot lookup metric names: %s\n", pmErrStr(sts));
      Platform_done();
      return false;
   }

   sts = pmLookupDescs(pcp->totalMetrics, pcp->pmids, pcp->descs);
   if (sts < 1) {
      if (sts < 0)
         fprintf(stderr, "Error: cannot lookup descriptors: %s\n", pmErrStr(sts));
      else /* ensure we have at least one valid metric to work with */
         fprintf(stderr, "Error: cannot find a single valid metric, exiting\n");
      Platform_done();
      return false;
   }

   /* set proc.control.perclient.threads to 1 for live contexts */
   Metric_enableThreads();

   /* extract values needed for setup - e.g. cpu count, pid_max */
   Metric_enable(PCP_PID_MAX, true);
   Metric_enable(PCP_BOOTTIME, true);
   Metric_enable(PCP_HINV_NCPU, true);
   Metric_enable(PCP_HINV_NDISK, true);
   Metric_enable(PCP_PERCPU_SYSTEM, true);
   Metric_enable(PCP_UNAME_SYSNAME, true);
   Metric_enable(PCP_UNAME_RELEASE, true);
   Metric_enable(PCP_UNAME_MACHINE, true);
   Metric_enable(PCP_UNAME_DISTRO, true);

   /* enable metrics for all dynamic columns (including those from dynamic screens) */
   for (size_t i = pcp->columns.offset; i < pcp->columns.offset + pcp->columns.count; i++)
      Metric_enable(i, true);

   Metric_fetch(NULL);

   for (Metric metric = 0; metric < PCP_PROC_PID; metric++)
      Metric_enable(metric, true);
   Metric_enable(PCP_PID_MAX, false); /* needed one time only */
   Metric_enable(PCP_BOOTTIME, false);
   Metric_enable(PCP_UNAME_SYSNAME, false);
   Metric_enable(PCP_UNAME_RELEASE, false);
   Metric_enable(PCP_UNAME_MACHINE, false);
   Metric_enable(PCP_UNAME_DISTRO, false);

   /* first sample (fetch) performed above, save constants */
   Platform_getBootTime();
   Platform_getRelease(0);
   Platform_getMaxCPU();
   Platform_getMaxPid();

   return true;
}

void Platform_dynamicColumnsDone(Hashtable* columns) {
   PCPDynamicColumns_done(columns);
}

void Platform_dynamicMetersDone(Hashtable* meters) {
   PCPDynamicMeters_done(meters);
}

void Platform_dynamicScreensDone(Hashtable* screens) {
   PCPDynamicScreens_done(screens);
}

void Platform_done(void) {
   pmDestroyContext(pcp->context);
   if (pcp->result)
      pmFreeResult(pcp->result);
   free(pcp->release);
   free(pcp->fetch);
   free(pcp->pmids);
   free(pcp->names);
   free(pcp->descs);
   free(pcp);
}

void Platform_setBindings(Htop_Action* keys) {
   /* no platform-specific key bindings */
   (void)keys;
}

int Platform_getUptime(void) {
   pmAtomValue value;
   if (Metric_values(PCP_UPTIME, &value, 1, PM_TYPE_32) == NULL)
      return 0;
   return value.l;
}

void Platform_getLoadAverage(double* one, double* five, double* fifteen) {
   *one = *five = *fifteen = 0.0;

   pmAtomValue values[3] = {0};
   if (Metric_values(PCP_LOAD_AVERAGE, values, 3, PM_TYPE_DOUBLE) != NULL) {
      *one = values[0].d;
      *five = values[1].d;
      *fifteen = values[2].d;
   }
}

unsigned int Platform_getMaxCPU(void) {
   if (pcp->ncpu)
      return pcp->ncpu;

   pmAtomValue value;
   if (Metric_values(PCP_HINV_NCPU, &value, 1, PM_TYPE_U32) != NULL)
      pcp->ncpu = value.ul;
   else
      pcp->ncpu = 1;
   return pcp->ncpu;
}

pid_t Platform_getMaxPid(void) {
   if (pcp->pidmax)
      return pcp->pidmax;

   pmAtomValue value;
   if (Metric_values(PCP_PID_MAX, &value, 1, PM_TYPE_32) == NULL)
      return INT_MAX;
   pcp->pidmax = value.l;
   return pcp->pidmax;
}

long long Platform_getBootTime(void) {
   if (pcp->btime)
      return pcp->btime;

   pmAtomValue value;
   if (Metric_values(PCP_BOOTTIME, &value, 1, PM_TYPE_64) != NULL)
      pcp->btime = value.ll;
   return pcp->btime;
}

static double Platform_setOneCPUValues(Meter* this, const Settings* settings, pmAtomValue* values) {
   unsigned long long value = values[CPU_TOTAL_PERIOD].ull;
   double total = (double) (value == 0 ? 1 : value);
   double percent;

   double* v = this->values;
   v[CPU_METER_NICE] = values[CPU_NICE_PERIOD].ull / total * 100.0;
   v[CPU_METER_NORMAL] = values[CPU_USER_PERIOD].ull / total * 100.0;
   if (settings->detailedCPUTime) {
      v[CPU_METER_KERNEL]  = values[CPU_SYSTEM_PERIOD].ull / total * 100.0;
      v[CPU_METER_IRQ]     = values[CPU_IRQ_PERIOD].ull / total * 100.0;
      v[CPU_METER_SOFTIRQ] = values[CPU_SOFTIRQ_PERIOD].ull / total * 100.0;
      this->curItems = 5;

      v[CPU_METER_STEAL]   = values[CPU_STEAL_PERIOD].ull / total * 100.0;
      v[CPU_METER_GUEST]   = values[CPU_GUEST_PERIOD].ull / total * 100.0;
      if (settings->accountGuestInCPUMeter) {
         this->curItems = 7;
      }

      v[CPU_METER_IOWAIT]  = values[CPU_IOWAIT_PERIOD].ull / total * 100.0;
   } else {
      v[CPU_METER_KERNEL] = values[CPU_SYSTEM_ALL_PERIOD].ull / total * 100.0;
      value = values[CPU_STEAL_PERIOD].ull + values[CPU_GUEST_PERIOD].ull;
      v[CPU_METER_IRQ] = value / total * 100.0;
      this->curItems = 4;
   }

   percent = sumPositiveValues(v, this->curItems);
   percent = MINIMUM(percent, 100.0);

   if (settings->detailedCPUTime) {
      this->curItems = 8;
   }

   v[CPU_METER_FREQUENCY] = values[CPU_FREQUENCY].d;
   v[CPU_METER_TEMPERATURE] = NAN;

   return percent;
}

double Platform_setCPUValues(Meter* this, int cpu) {
   const PCPMachine* phost = (const PCPMachine*) this->host;
   const Settings* settings = this->host->settings;

   if (cpu <= 0) /* use aggregate values */
      return Platform_setOneCPUValues(this, settings, phost->cpu);
   return Platform_setOneCPUValues(this, settings, phost->percpu[cpu - 1]);
}

void Platform_setMemoryValues(Meter* this) {
   const Machine* host = this->host;
   const PCPMachine* phost = (const PCPMachine*) host;

   this->total = host->totalMem;
   this->values[MEMORY_METER_USED] = host->usedMem;
   this->values[MEMORY_METER_SHARED] = host->sharedMem;
   this->values[MEMORY_METER_COMPRESSED] = 0;
   this->values[MEMORY_METER_BUFFERS] = host->buffersMem;
   this->values[MEMORY_METER_CACHE] = host->cachedMem;
   this->values[MEMORY_METER_AVAILABLE] = host->availableMem;

   if (phost->zfs.enabled != 0) {
      // ZFS does not shrink below the value of zfs_arc_min.
      unsigned long long int shrinkableSize = 0;
      if (phost->zfs.size > phost->zfs.min)
         shrinkableSize = phost->zfs.size - phost->zfs.min;
      this->values[MEMORY_METER_USED] -= shrinkableSize;
      this->values[MEMORY_METER_CACHE] += shrinkableSize;
      this->values[MEMORY_METER_AVAILABLE] += shrinkableSize;
   }

   if (phost->zswap.usedZswapOrig > 0 || phost->zswap.usedZswapComp > 0) {
      this->values[MEMORY_METER_USED] -= phost->zswap.usedZswapComp;
      this->values[MEMORY_METER_COMPRESSED] += phost->zswap.usedZswapComp;
   }
}

void Platform_setSwapValues(Meter* this) {
   const Machine* host = this->host;
   const PCPMachine* phost = (const PCPMachine*) host;

   this->total = host->totalSwap;
   this->values[SWAP_METER_USED] = host->usedSwap;
   this->values[SWAP_METER_CACHE] = host->cachedSwap;
   this->values[SWAP_METER_FRONTSWAP] = 0; /* frontswap -- memory that is accounted to swap but resides elsewhere */

   if (phost->zswap.usedZswapOrig > 0 || phost->zswap.usedZswapComp > 0) {
      /* refer to linux/Platform.c::Platform_setSwapValues for details */
      this->values[SWAP_METER_USED] -= phost->zswap.usedZswapOrig;
      if (this->values[SWAP_METER_USED] < 0) {
         /* subtract the overflow from SwapCached */
         this->values[SWAP_METER_CACHE] += this->values[SWAP_METER_USED];
         this->values[SWAP_METER_USED] = 0;
      }
      this->values[SWAP_METER_FRONTSWAP] += phost->zswap.usedZswapOrig;
   }
}

void Platform_setZramValues(Meter* this) {
   int i, count = Metric_instanceCount(PCP_ZRAM_CAPACITY);
   if (!count) {
      this->total = 0;
      this->values[0] = 0;
      this->values[1] = 0;
      return;
   }

   pmAtomValue* values = xCalloc(count, sizeof(pmAtomValue));
   ZramStats stats = {0};

   if (Metric_values(PCP_ZRAM_CAPACITY, values, count, PM_TYPE_U64)) {
      for (i = 0; i < count; i++)
         stats.totalZram += values[i].ull;
   }
   if (Metric_values(PCP_ZRAM_ORIGINAL, values, count, PM_TYPE_U64)) {
      for (i = 0; i < count; i++)
         stats.usedZramOrig += values[i].ull;
   }
   if (Metric_values(PCP_ZRAM_COMPRESSED, values, count, PM_TYPE_U64)) {
      for (i = 0; i < count; i++)
         stats.usedZramComp += values[i].ull;
   }

   free(values);

   if (stats.usedZramComp > stats.usedZramOrig) {
      stats.usedZramComp = stats.usedZramOrig;
   }

   this->total = stats.totalZram;
   this->values[0] = stats.usedZramComp;
   this->values[1] = stats.usedZramOrig - stats.usedZramComp;
}

void Platform_setZfsArcValues(Meter* this) {
   const PCPMachine* phost = (const PCPMachine*) this->host;

   ZfsArcMeter_readStats(this, &phost->zfs);
}

void Platform_setZfsCompressedArcValues(Meter* this) {
   const PCPMachine* phost = (const PCPMachine*) this->host;

   ZfsCompressedArcMeter_readStats(this, &phost->zfs);
}

void Platform_getHostname(char* buffer, size_t size) {
   const char* hostname = pmGetContextHostName(pcp->context);
   String_safeStrncpy(buffer, hostname, size);
}

void Platform_getRelease(char** string) {
   /* fast-path - previously-formatted string */
   if (string) {
      *string = pcp->release;
      return;
   }

   /* first call, extract just-sampled values */
   pmAtomValue sysname, release, machine, distro;
   if (!Metric_values(PCP_UNAME_SYSNAME, &sysname, 1, PM_TYPE_STRING))
      sysname.cp = NULL;
   if (!Metric_values(PCP_UNAME_RELEASE, &release, 1, PM_TYPE_STRING))
      release.cp = NULL;
   if (!Metric_values(PCP_UNAME_MACHINE, &machine, 1, PM_TYPE_STRING))
      machine.cp = NULL;
   if (!Metric_values(PCP_UNAME_DISTRO, &distro, 1, PM_TYPE_STRING))
      distro.cp = NULL;

   size_t length = 16; /* padded for formatting characters */
   if (sysname.cp)
      length += strlen(sysname.cp);
   if (release.cp)
      length += strlen(release.cp);
   if (machine.cp)
      length += strlen(machine.cp);
   if (distro.cp)
      length += strlen(distro.cp);
   pcp->release = xCalloc(1, length);

   if (sysname.cp) {
      strcat(pcp->release, sysname.cp);
      strcat(pcp->release, " ");
   }
   if (release.cp) {
      strcat(pcp->release, release.cp);
      strcat(pcp->release, " ");
   }
   if (machine.cp) {
      strcat(pcp->release, "[");
      strcat(pcp->release, machine.cp);
      strcat(pcp->release, "] ");
   }
   if (distro.cp) {
      if (pcp->release[0] != '\0') {
         strcat(pcp->release, "@ ");
         strcat(pcp->release, distro.cp);
      } else {
         strcat(pcp->release, distro.cp);
      }
      strcat(pcp->release, " ");
   }

   if (pcp->release) /* cull trailing space */
      pcp->release[strlen(pcp->release)] = '\0';

   free(distro.cp);
   free(machine.cp);
   free(release.cp);
   free(sysname.cp);
}

char* Platform_getProcessEnv(pid_t pid) {
   pmAtomValue value;
   if (!Metric_instance(PCP_PROC_ENVIRON, pid, 0, &value, PM_TYPE_STRING))
      return NULL;
   return value.cp;
}

FileLocks_ProcessData* Platform_getProcessLocks(pid_t pid) {
   (void)pid;
   return NULL;
}

void Platform_getPressureStall(const char* file, bool some, double* ten, double* sixty, double* threehundred) {
   *ten = *sixty = *threehundred = 0;

   Metric metric;
   if (String_eq(file, "cpu"))
      metric = PCP_PSI_CPUSOME;
   else if (String_eq(file, "io"))
      metric = some ? PCP_PSI_IOSOME : PCP_PSI_IOFULL;
   else if (String_eq(file, "irq"))
      metric = PCP_PSI_IRQFULL;
   else if (String_eq(file, "mem"))
      metric = some ? PCP_PSI_MEMSOME : PCP_PSI_MEMFULL;
   else
      return;

   pmAtomValue values[3] = {0};
   if (Metric_values(metric, values, 3, PM_TYPE_DOUBLE) != NULL) {
      *ten = values[0].d;
      *sixty = values[1].d;
      *threehundred = values[2].d;
   }
}

bool Platform_getDiskIO(DiskIOData* data) {
   memset(data, 0, sizeof(*data));

   pmAtomValue value;
   if (Metric_values(PCP_DISK_READB, &value, 1, PM_TYPE_U64) != NULL)
      data->totalBytesRead = value.ull;
   if (Metric_values(PCP_DISK_WRITEB, &value, 1, PM_TYPE_U64) != NULL)
      data->totalBytesWritten = value.ull;
   if (Metric_values(PCP_DISK_ACTIVE, &value, 1, PM_TYPE_U64) != NULL)
      data->totalMsTimeSpend = value.ull;
   if (Metric_values(PCP_HINV_NDISK, &value, 1, PM_TYPE_U64) != NULL)
      data->numDisks = value.ull;
   return true;
}

bool Platform_getNetworkIO(NetworkIOData* data) {
   pmAtomValue value;
   if (Metric_values(PCP_NET_RECVB, &value, 1, PM_TYPE_U64) != NULL)
      data->bytesReceived = value.ull;
   if (Metric_values(PCP_NET_SENDB, &value, 1, PM_TYPE_U64) != NULL)
      data->bytesTransmitted = value.ull;
   if (Metric_values(PCP_NET_RECVP, &value, 1, PM_TYPE_U64) != NULL)
      data->packetsReceived = value.ull;
   if (Metric_values(PCP_NET_SENDP, &value, 1, PM_TYPE_U64) != NULL)
      data->packetsTransmitted = value.ull;
   return true;
}

void Platform_getFileDescriptors(double* used, double* max) {
   *used = NAN;
   *max = 65536;

   pmAtomValue value;
   if (Metric_values(PCP_VFS_FILES_COUNT, &value, 1, PM_TYPE_32) != NULL)
      *used = value.l;
   if (Metric_values(PCP_VFS_FILES_MAX, &value, 1, PM_TYPE_32) != NULL)
      *max = value.l;
}

void Platform_getBattery(double* level, ACPresence* isOnAC) {
   *level = NAN;
   *isOnAC = AC_ERROR;
}

const char* Platform_getFailedState(void) {
   return pcp->reconnect ? "PMCD DOWN" : NULL;
}

void Platform_longOptionsUsage(ATTR_UNUSED const char* name) {
   printf(
"   --host=HOSTSPEC              metrics source is PMCD at HOSTSPEC [see PCPIntro(1)]\n"
"   --hostzone                   set reporting timezone to local time of metrics source\n"
"   --timezone=TZ                set reporting timezone\n");
}

CommandLineStatus Platform_getLongOption(int opt, ATTR_UNUSED int argc, char** argv) {
   /* libpcp export without a header definition */
   extern void __pmAddOptHost(pmOptions*, char*);

   switch (opt) {
      case PLATFORM_LONGOPT_HOST:  /* --host=HOSTSPEC */
         if (argv[optind][0] == '\0')
            return STATUS_ERROR_EXIT;
         __pmAddOptHost(&opts, optarg);
         return STATUS_OK;

      case PLATFORM_LONGOPT_HOSTZONE:  /* --hostzone */
         if (opts.timezone) {
            pmprintf("%s: at most one of -Z and -z allowed\n", pmGetProgname());
            opts.errors++;
         } else {
            opts.tzflag = 1;
         }
         return STATUS_OK;

      case PLATFORM_LONGOPT_TIMEZONE:  /* --timezone=TZ */
         if (argv[optind][0] == '\0')
            return STATUS_ERROR_EXIT;
         if (opts.tzflag) {
            pmprintf("%s: at most one of -Z and -z allowed\n", pmGetProgname());
            opts.errors++;
         } else {
            opts.timezone = optarg;
         }
         return STATUS_OK;

      default:
         break;
   }

   return STATUS_ERROR_EXIT;
}

void Platform_gettime_realtime(struct timeval* tv, uint64_t* msec) {
   if (gettimeofday(tv, NULL) == 0) {
      /* shift by start offset to stay in lock-step with realtime (archives) */
      if (pcp->offset.tv_sec || pcp->offset.tv_usec)
         pmtimevalDec(tv, &pcp->offset);
      *msec = ((uint64_t)tv->tv_sec * 1000) + ((uint64_t)tv->tv_usec / 1000);
   } else {
      memset(tv, 0, sizeof(struct timeval));
      *msec = 0;
   }
}

void Platform_gettime_monotonic(uint64_t* msec) {
   if (pcp->result) {
#if PMAPI_VERSION >= 3
      *msec = ((uint64_t)pcp->result->timestamp.tv_sec * 1000) + ((uint64_t)pcp->result->timestamp.tv_nsec / 1000000);
#else
      *msec = ((uint64_t)pcp->result->timestamp.tv_sec * 1000) + ((uint64_t)pcp->result->timestamp.tv_usec / 1000);
#endif
   } else {
      *msec = 0;
   }
}

Hashtable* Platform_dynamicMeters(void) {
   return pcp->meters.table;
}

void Platform_dynamicMeterInit(Meter* meter) {
   PCPDynamicMeter* this = Hashtable_get(pcp->meters.table, meter->param);
   if (this)
      PCPDynamicMeter_enable(this);
}

void Platform_dynamicMeterUpdateValues(Meter* meter) {
   PCPDynamicMeter* this = Hashtable_get(pcp->meters.table, meter->param);
   if (this)
      PCPDynamicMeter_updateValues(this, meter);
}

void Platform_dynamicMeterDisplay(const Meter* meter, RichString* out) {
   PCPDynamicMeter* this = Hashtable_get(pcp->meters.table, meter->param);
   if (this)
      PCPDynamicMeter_display(this, meter, out);
}

Hashtable* Platform_dynamicColumns(void) {
   return pcp->columns.table;
}

const char* Platform_dynamicColumnName(unsigned int key) {
   PCPDynamicColumn* this = Hashtable_get(pcp->columns.table, key);
   if (this) {
      Metric_enable(this->id, true);
      if (this->super.caption)
         return this->super.caption;
      if (this->super.heading)
         return this->super.heading;
      return this->super.name;
   }
   return NULL;
}

bool Platform_dynamicColumnWriteField(const Process* proc, RichString* str, unsigned int key) {
   PCPDynamicColumn* this = Hashtable_get(pcp->columns.table, key);
   if (this) {
      PCPDynamicColumn_writeField(this, proc, str);
      return true;
   }
   return false;
}

Hashtable* Platform_dynamicScreens(void) {
   return pcp->screens.table;
}

void Platform_defaultDynamicScreens(Settings* settings) {
   PCPDynamicScreen_appendScreens(&pcp->screens, settings);
}

void Platform_addDynamicScreen(ScreenSettings* ss) {
   PCPDynamicScreen_addDynamicScreen(&pcp->screens, ss);
}

void Platform_addDynamicScreenAvailableColumns(Panel* availableColumns, const char* screen) {
   Hashtable* screens = pcp->screens.table;
   PCPDynamicScreens_addAvailableColumns(availableColumns, screens, screen);
}

void Platform_updateTables(Machine* host) {
   PCPDynamicScreen_appendTables(&pcp->screens, host);
   PCPDynamicColumns_setupWidths(&pcp->columns);
}
