/*
 * Copyright (c) 1993, 1994 Barton P. Miller, Jeff Hollingsworth,
 *     Bruce Irvin, Jon Cargille, Krishna Kunchithapadam, Karen
 *     Karavanic, Tia Newhall, Mark Callaghan.  All rights reserved.
 * 
 * This software is furnished under the condition that it may not be
 * provided or otherwise made available to, or used by, any other
 * person, except as provided for by the terms of applicable license
 * agreements.  No title to or ownership of the software is hereby
 * transferred.  The name of the principals may not be used in any
 * advertising or publicity related to this software without specific,
 * written prior authorization.  Any use of this software must include
 * the above copyright notice.
 *
 */





/************************************************************************
 * RTposix.c: runtime instrumentation functions for generic posix.
************************************************************************/





/************************************************************************
 * header files.
************************************************************************/

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "kludges.h"
#include "rtinst/h/rtinst.h"
#include "rtinst/h/trace.h"





/************************************************************************
 * symbolic constants.
************************************************************************/

static const double MILLION = 1000000.0;





/************************************************************************
 * external functions.
************************************************************************/

extern void   DYNINSTos_init(void);
extern time64 DYNINSTgetCPUtime(void);
extern time64 DYNINSTgetWalltime(void);





/************************************************************************
 * time64 DYNINSTgetUserTime(void)
 *
 * get the user time for "an" LWP of the monitored process.
************************************************************************/

time64
DYNINSTgetUserTime(void) {
    return DYNINSTgetCPUtime();
}





/************************************************************************
 * void DYNINSTbreakPoint(void)
 *
 * stop oneself.
************************************************************************/

void
DYNINSTbreakPoint(void) {
    kill(getpid(), SIGSTOP);
}





/************************************************************************
 * void DYNINSTstartProcessTimer(tTimer* timer)
************************************************************************/

void
DYNINSTstartProcessTimer(tTimer* timer) {
    if (timer->counter == 0) {
        timer->start     = DYNINSTgetUserTime();
        timer->normalize = MILLION;
    }
    timer->counter++;
}





/************************************************************************
 * void DYNINSTstopProcessTimer(tTimer* timer)
************************************************************************/

void
DYNINSTstopProcessTimer(tTimer* timer) {
    if (!timer->counter) {
        return;
    }

    if (timer->counter == 1) {
        time64 now = DYNINSTgetUserTime();

        timer->snapShot = now - timer->start + timer->total;
        timer->mutex    = 1;
        timer->counter  = 0;
        timer->total    = DYNINSTgetUserTime() - timer->start + timer->total;
        timer->mutex    = 0;

        if (now < timer->start) {
            printf("process timer rollback\n"); fflush(stdout);
            abort();
        }
    }
    else {
        timer->counter--;
    }
}





/************************************************************************
 * void DYNINSTstartWallTimer(tTimer* timer)
************************************************************************/

void
DYNINSTstartWallTimer(tTimer* timer) {
    if (timer->counter == 0) {
        timer->start     = DYNINSTgetWalltime();
        timer->normalize = MILLION;
    }
    timer->counter++;
}





/************************************************************************
 * void DYNINSTstopWallTimer(tTimer* timer)
************************************************************************/

void
DYNINSTstopWallTimer(tTimer* timer) {
    if (!timer->counter) {
        return;
    }

    if (timer->counter == 1) {
        time64 now = DYNINSTgetWalltime();

        timer->snapShot = now - timer->start + timer->total;
        timer->mutex    = 1;
        timer->counter  = 0;
        timer->total    = DYNINSTgetWalltime() - timer->start + timer->total;
        timer->mutex    = 0;

        if (now < timer->start) {
            printf("wall timer rollback\n"); fflush(stdout);
            abort();
        }
    }
    else {
        timer->counter--;
    }
}





/************************************************************************
 * void DYNINSTpauseProcess(void)
 * void DYNINSTcontinueProcess(void)
 *
 * pause and continue process.
************************************************************************/

static volatile int DYNINSTpauseDone = 0;

void
DYNINSTpauseProcess(void) {
    DYNINSTpauseDone = 0;
    while (!DYNINSTpauseDone) {
    }
}

void
DYNINSTcontinueProcess(void) {
    DYNINSTpauseDone = 1;
}





/************************************************************************
 * void install_ualarm(unsigned value, unsigned interval)
 *
 * an implementation of "ualarm" using the "setitimer" syscall.
************************************************************************/

static void
install_ualarm(unsigned value, unsigned interval) {
    struct itimerval it;

    it.it_value.tv_sec     = it.it_interval.tv_sec = 0;
    it.it_value.tv_usec    = value;
    it.it_interval.tv_usec = interval;

    if (setitimer(ITIMER_REAL, &it, 0) == -1) {
        perror("setitimer");
        abort();
    }
}





/************************************************************************
 * global data for DYNINST functions.
************************************************************************/

double DYNINSTdata[SYN_INST_BUF_SIZE/sizeof(double)];
double DYNINSTglobalData[SYN_INST_BUF_SIZE/sizeof(double)];





/************************************************************************
 * float DYNINSTcyclesPerSecond(void)
 *
 * need a well-defined method for finding the CPU cycle speed
 * on each CPU.
************************************************************************/

#define NOPS_4  asm("nop"); asm("nop"); asm("nop"); asm("nop")
#define NOPS_16 NOPS_4; NOPS_4; NOPS_4; NOPS_4

static float
DYNINSTcyclesPerSecond(void) {
    int            i;
    time64         start_cpu;
    time64         end_cpu;
    double         elapsed;
    double         speed;
    const unsigned LOOP_LIMIT = 50000;

    start_cpu = DYNINSTgetCPUtime();
    for (i = 0; i < LOOP_LIMIT; i++) {
        NOPS_16; NOPS_16; NOPS_16; NOPS_16;
        NOPS_16; NOPS_16; NOPS_16; NOPS_16;
        NOPS_16; NOPS_16; NOPS_16; NOPS_16;
        NOPS_16; NOPS_16; NOPS_16; NOPS_16;
    }
    end_cpu = DYNINSTgetCPUtime();
    elapsed = (double) end_cpu - start_cpu;
    speed   = (MILLION*256*LOOP_LIMIT)/elapsed;

    printf("elapsed = %f\n", elapsed);
    printf("speed   = %f\n", speed);

    return speed;
}





/************************************************************************
 * void saveFPUstate(float* base)
 * void restoreFPUstate(float* base)
 *
 * save and restore state of FPU on signals.  these are null functions
 * for most well designed and implemented systems.
************************************************************************/

static void
saveFPUstate(float* base) {
}

static void
restoreFPUstate(float* base) {
}





#ifdef notdef
/* removed this code until the mapped counter code is finished - jkh 10/26/94 */

/************************************************************************
 * int64 DYNINSTgetObservedCycles(Boolean in_signal)
 *
 * report the observed cost of instrumentation in machine cycles.
 * do not use any of the reserved registers.
************************************************************************/

static int64
DYNINSTgetObservedCycles(Boolean in_signal) {
    static int64    value = 0;
    static unsigned lowBits = 0;

    if (in_signal) {
        return value;
    }

    value += lowBits;
    return value;
}
#else
extern int64 DYNINSTgetObservedCycles(Boolean in_signal);
#endif





/************************************************************************
 * void DYNINSTsampleValues(void)
 *
 * dummy function for sampling timers and counters.  the actual code
 * is added by dynamic instrumentation from the paradyn daemons.
************************************************************************/

static int DYNINSTnumReported = 0;

void
DYNINSTsampleValues(void) {
    DYNINSTnumReported++;
}





/************************************************************************
 * void DYNINSTflushTrace(void)
 *
 * flush any accumalated traces.
************************************************************************/

static FILE* DYNINSTtraceFp = 0;

static void
DYNINSTflushTrace(void) {
    if (DYNINSTtraceFp) fflush(DYNINSTtraceFp);
}





/************************************************************************
 * void DYNINSTgenerateTraceRecord(traceStream sid, short type,
 *                                 short length, void* data, int flush)
************************************************************************/

static time64 startWall = 0;

void
DYNINSTgenerateTraceRecord(traceStream sid, short type, short length,
    void *eventData, int flush) {
    int             ret;
    static unsigned pipe_gone = 0;
    traceHeader     header;
    int             count;
    char            buffer[1024];

    if (pipe_gone) {
        return;
    }

    header.wall    = DYNINSTgetWalltime() - startWall;
    header.process = DYNINSTgetCPUtime();

    length = ALIGN_TO_WORDSIZE(length);

    header.type   = type;
    header.length = length;

    count = 0;
    memcpy(&buffer[count], &sid, sizeof(traceStream));
    count += sizeof(traceStream);

    memcpy(&buffer[count], &header, sizeof(header));
    count += sizeof(header);

    count = ALIGN_TO_WORDSIZE(count);
    memcpy(&buffer[count], eventData, length);
    count += length;

    errno = 0;

    if (!DYNINSTtraceFp || (type == TR_EXIT)) {
        DYNINSTtraceFp = fdopen(dup(CONTROLLER_FD), "w");
    }

    ret = fwrite(buffer, count, 1, DYNINSTtraceFp);
    if (ret != 1) {
        printf("unable to write trace record, errno=%d\n", errno);
        printf("disabling further data logging, pid=%d\n", (int) getpid());
        fflush(stdout);
        pipe_gone = 1;
    }
    if (flush) DYNINSTflushTrace();
}





/************************************************************************
 * void DYNINSTreportBaseTramps(void)
 *
 * report the cost of base trampolines.
************************************************************************/

static float DYNINSTcyclesToUsec  = 0;
static time64 DYNINSTlastWallTime = 0;
static time64 DYNINSTlastCPUTime  = 0;

void
DYNINSTreportBaseTramps() {
    costUpdate sample;
    time64     currentCPU;
    time64     currentWall;
    time64     elapsedWallTime;
    time64     currentPauseTime;

    sample.slotsExecuted = 0;
    sample.observedCost  = ((double) DYNINSTgetObservedCycles(1)) *
        (DYNINSTcyclesToUsec / MILLION);


    currentCPU       = DYNINSTgetCPUtime();
    currentWall      = DYNINSTgetWalltime();
    elapsedWallTime  = currentWall - DYNINSTlastWallTime;
    currentPauseTime = elapsedWallTime - (currentCPU - DYNINSTlastCPUTime);

    sample.pauseTime  = ((double) currentPauseTime);
    sample.pauseTime /= 1000000.0;

    DYNINSTlastWallTime = currentWall;
    DYNINSTlastCPUTime  = currentCPU;

    DYNINSTgenerateTraceRecord(0, TR_COST_UPDATE, sizeof(sample), &sample, 0);
}





/************************************************************************
 * void DYNINSTalarmExpire(void)
 *
 * called periodically by signal handlers.  report sampled data back
 * to the paradyn daemons.  when the program exits, DYNINSTsampleValues
 * should be called directly.
************************************************************************/

#define N_FP_REGS 33

static volatile int DYNINSTsampleMultiple    = 1;
static int          DYNINSTnumSampled        = 0;
static int          DYNINSTtotalAlarmExpires = 0;
static time64       DYNINSTtotalSampleTime   = 0;

void
DYNINSTalarmExpire(int signo) {
    time64     start_cpu;
    time64     end_cpu;
    static int in_sample = 0;
    float      fp_context[N_FP_REGS];

    if (in_sample) {
        return;
    }
    in_sample = 1;

    DYNINSTtotalAlarmExpires++;
    if ((++DYNINSTnumSampled % DYNINSTsampleMultiple) == 0) {
        saveFPUstate(fp_context);
        start_cpu = DYNINSTgetCPUtime();

        /* to keep observed cost accurate due to 32-cycle rollover */
        (void) DYNINSTgetObservedCycles(0);

        DYNINSTsampleValues();
        DYNINSTreportBaseTramps();
        DYNINSTflushTrace();

        end_cpu = DYNINSTgetCPUtime();
        DYNINSTtotalSampleTime += (end_cpu - start_cpu);
        restoreFPUstate(fp_context);
    }

    in_sample = 0;
}





/************************************************************************
 * void DYNINSTinit(int doskip)
 *
 * initialize the DYNINST library.  this function is called at the start
 * of the application program.
 *
 * the first this to do is to call the os specific initialization
 * function.
************************************************************************/

static float  DYNINSTsamplingRate   = 0;
static int    DYNINSTtotalSamples   = 0;
static tTimer DYNINSTelapsedCPUTime;
static tTimer DYNINSTelapsedTime;

void
DYNINSTinit(int doskip) {
    struct sigaction act;
    unsigned         val;
    const char*      interval;

    DYNINSTos_init();

    startWall = 0;

    DYNINSTcyclesToUsec = MILLION/DYNINSTcyclesPerSecond();
    DYNINSTlastCPUTime  = DYNINSTgetCPUtime();
    DYNINSTlastWallTime = DYNINSTgetWalltime();

    act.sa_handler = DYNINSTalarmExpire;
    act.sa_flags   = 0;
#if defined(SA_INTERRUPT)
    act.sa_flags  |= SA_INTERRUPT;
#endif /* defined(SA_INTERRUPT) */
    sigfillset(&act.sa_mask);

    if (sigaction(SIGALRM, &act, 0) == -1) {
        perror("sigaction(SIGALRM)");
        abort();
    }

    val = 500000;
    interval = getenv("DYNINSTsampleInterval");
    if (interval) {
        val = atoi(interval);
    }
    DYNINSTsamplingRate = val/1000000.0;

    install_ualarm(val, val);

    printf("Time at main %g us\n", (double) DYNINSTgetCPUtime());
    if (!doskip) {
        DYNINSTbreakPoint();
    }

    DYNINSTstartWallTimer(&DYNINSTelapsedTime);
    DYNINSTstartProcessTimer(&DYNINSTelapsedCPUTime);
}





/************************************************************************
 * void DYNINSTexit(void)
 *
 * handle `exit' in the application. current nothing is done.
************************************************************************/

void
DYNINSTexit(void) {
}





/************************************************************************
 * void DYNINSTreportTimer(tTimer* timer)
 *
 * report the timer `timer' to the paradyn daemon.
************************************************************************/

void
DYNINSTreportTimer(tTimer *timer) {
    time64 now = 0;
    time64 total;
    traceSample sample;

    if (timer->mutex) {
        total = timer->snapShot;
    }
    else if (timer->counter) {
        /* timer is running */
        if (timer->type == processTime) {
            now = DYNINSTgetUserTime();
        } else {
            now = DYNINSTgetWalltime();
        }
        total = now - timer->start + timer->total;
    }
    else {
        total = timer->total;
    }

    if (total < timer->lastValue) {
        if (timer->type == processTime) {
            printf("process ");
        }
        else {
            printf("wall ");
        }
        printf("time regressed timer %d, total = %f, last = %f\n",
            timer->id.id, (float) total, (float) timer->lastValue);
        if (timer->counter) {
            printf("timer was active\n");
        } else {
            printf("timer was inactive\n");
        }
        printf("mutex=%d, counter=%d, sampled=%d, snapShot=%f\n",
            (int) timer->mutex, (int) timer->counter, (int) timer->sampled,
            (double) timer->snapShot);
        printf("now = %f, start = %f, total = %f\n",
            (double) now, (double) timer->start, (double) timer->total);
        fflush(stdout);
        abort();
    }

    timer->lastValue = total;

    sample.id = timer->id;
    sample.value = ((double) total) / (double) timer->normalize;
    DYNINSTtotalSamples++;

    DYNINSTgenerateTraceRecord(0, TR_SAMPLE, sizeof(sample), &sample, 0);
    /* printf("raw sample %d = %f\n", sample.id.id, sample.value); */
}





/************************************************************************
 * void DYNINSTfork(void* arg, int pid)
 *
 * track a fork() system call, and report to the paradyn daemon.
************************************************************************/

void
DYNINSTfork(void* arg, int pid) {
    int sid = 0;
    traceFork forkRec;

    printf("fork called with pid = %d\n", pid);
    fflush(stdout);
    if (pid > 0) {
        forkRec.ppid   = getpid();
        forkRec.pid    = pid;
        forkRec.npids  = 1;
        forkRec.stride = 0;
        DYNINSTgenerateTraceRecord(sid,TR_FORK,sizeof(forkRec), &forkRec, 1);
    } else {
        DYNINSTinit(1);
    }
}





/************************************************************************
 * void DYNINSTprintCost(void)
 *
 * print a detailed summary of the cost of the application's run.
************************************************************************/

void
DYNINSTprintCost(void) {
    FILE *fp;
    time64 now;
    int64 value;
    struct endStatsRec stats;

    DYNINSTstopProcessTimer(&DYNINSTelapsedCPUTime);
    DYNINSTstopWallTimer(&DYNINSTelapsedTime);

    value = DYNINSTgetObservedCycles(0);
    stats.instCycles = value;

    value *= DYNINSTcyclesToUsec;

    stats.alarms      = DYNINSTtotalAlarmExpires;
    stats.numReported = DYNINSTnumReported;
    stats.instTime    = ((double) value)/MILLION;
    stats.handlerCost = ((double) DYNINSTtotalSampleTime)/MILLION;

    now = DYNINSTgetCPUtime();
    stats.totalCpuTime  = ((double) DYNINSTelapsedCPUTime.total)/MILLION;
    stats.totalWallTime = ((double) DYNINSTelapsedTime.total/MILLION);

    stats.samplesReported = DYNINSTtotalSamples;
    stats.samplingRate    = DYNINSTsamplingRate;

    stats.userTicks = 0;
    stats.instTicks = 0;

    fp = fopen("stats.out", "w");

    fprintf(fp, "DYNINSTtotalAlarmExpires %d\n", stats.alarms);
    fprintf(fp, "DYNINSTnumReported %d\n", stats.numReported);
    fprintf(fp,"Raw cycle count = %f\n", (double) stats.instCycles);
    fprintf(fp,"Total instrumentation cost = %f\n", stats.instTime);
    fprintf(fp,"Total handler cost = %f\n", stats.handlerCost);
    fprintf(fp,"Total cpu time of program %f\n", stats.totalCpuTime);
    fprintf(fp,"Elapsed wall time of program %f\n",
        stats.totalWallTime/1000000.0);
    fprintf(fp,"total data samples %d\n", stats.samplesReported);
    fprintf(fp,"sampling rate %f\n", stats.samplingRate);
    fprintf(fp,"Application program ticks %d\n", stats.userTicks);
    fprintf(fp,"Instrumentation ticks %d\n", stats.instTicks);

    fclose(fp);

    /* record that we are done -- should be somewhere better. */
    DYNINSTgenerateTraceRecord(0, TR_EXIT, sizeof(stats), &stats, 1);
}





/************************************************************************
 * void DYNINSTrecordTag(int tag)
 *
 * mark a new tag in tag list.
************************************************************************/

static int DYNINSTtagCount = 0;
static int DYNINSTtagLimit = 1000;
static int DYNINSTtags[1000];

void
DYNINSTrecordTag(int tag) {
    int i;

    for (i=0; i < DYNINSTtagCount; i++) {
        if (DYNINSTtags[i] == tag) return;
    }

    if (DYNINSTtagCount == DYNINSTtagLimit) abort();
    DYNINSTtags[DYNINSTtagCount++] = tag;
}





/************************************************************************
 * void DYNINSTreportNewTags(void)
 *
 * inform the paradyn daemons of new message tags.
************************************************************************/

void
DYNINSTreportNewTags(void) {
    int i;
    static int lastTagCount;
    struct _newresource newRes;

    for (i=lastTagCount; i < DYNINSTtagCount; i++) {
        memset(&newRes, '\0', sizeof(newRes));
        sprintf(newRes.name, "SyncObject/MsgTag/%d", DYNINSTtags[i]);
        strcpy(newRes.abstraction, "BASE");
        DYNINSTgenerateTraceRecord(0, TR_NEW_RESOURCE, 
            sizeof(struct _newresource), &newRes, 1);
    }
    lastTagCount = DYNINSTtagCount;
}





/************************************************************************
 * void DYNINSTreportCounter(intCounter* counter)
 *
 * report value of counter to paradynd.
************************************************************************/

void
DYNINSTreportCounter(intCounter* counter) {
    traceSample sample;

    sample.value = counter->value;
    sample.id    = counter->id;
    DYNINSTtotalSamples++;

    DYNINSTgenerateTraceRecord(0, TR_SAMPLE, sizeof(sample), &sample, 0);
}





/************************************************************************
 * DYNINST test functions.
************************************************************************/

void DYNINSTsimplePrint(void) {
    printf("inside dynamic inst function\n");
}

void DYNINSTentryPrint(int arg) {
    printf("enter %d\n", arg);
}

void DYNINSTcallFrom(int arg) {
    printf("call from %d\n", arg);
}

void DYNINSTcallReturn(int arg) {
    printf("return to %d\n", arg);
}

void DYNINSTexitPrint(int arg) {
    printf("exit %d\n", arg);
}





/************************************************************************
 * void DYNINSTreportCost(intCounter* counter)
 *
 * report the cost (from the cost model).
************************************************************************/

void
DYNINSTreportCost(intCounter *counter) {
    int64         value;
    double        cost;
    static double prev_cost = 0;
    traceSample   sample;

    value = DYNINSTgetObservedCycles(1);
    cost  = ((double) value) * (DYNINSTcyclesToUsec / MILLION);
    if (cost < prev_cost) {
        fprintf(stderr, "Fatal Error: cost counter went backwards\n");
        fflush(stderr);
        abort();
    }

    prev_cost = cost;

    sample.value = cost;
    sample.id    = counter->id;
    DYNINSTtotalSamples++;

    DYNINSTgenerateTraceRecord(0, TR_SAMPLE, sizeof sample, &sample, 0);
}
