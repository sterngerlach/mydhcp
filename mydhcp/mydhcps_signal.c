
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcps_alarm.c */

#include <errno.h>
#include <string.h>
#include <sys/time.h>

#include "mydhcps_alarm.h"
#include "util.h"

/*
 * シグナルSIGALRMが発生したかどうかのフラグ
 */
volatile sig_atomic_t is_sigalrm_handled;

/*
 * シグナルSIGALRMをブロックするためのシグナルマスク
 */
static sigset_t block_sigset;

/*
 * 以前のシグナルマスク
 */
static sigset_t old_sigset;

/*
 * シグナルSIGALRMのハンドラ
 */
void sigalrm_handler(int sig)
{
    (void)sig;

    is_sigalrm_handled = 1;
}

/*
 * シグナルSIGALRMのハンドラを設定
 */
bool setup_sigalrm_handler()
{
    struct sigaction sigact;

    /* シグナルSIGALRMのハンドラを設定 */
    sigact.sa_handler = sigalrm_handler;
    sigact.sa_flags = 0;
    sigemptyset(&sigact.sa_mask);
    
    if (sigaction(SIGALRM, &sigact, NULL) < 0) {
        print_error(__func__, "sigaction() failed: %s\n", strerror(errno));
        return false;
    }

    return true;
}

/*
 * シグナルSIGALRMをブロック
 */
bool block_sigalrm()
{
    static bool block_sigset_initialized = false;
    
    if (!block_sigset_initialized) {
        /* シグナルSIGALRMをブロックするためのシグナルマスクを作成 */
        sigemptyset(&block_sigset);
        sigaddset(&block_sigset, SIGALRM);
        block_sigset_initialized = true;
    }

    /* シグナルSIGALRMをブロック */
    if (sigprocmask(SIG_BLOCK, &block_sigset, &old_sigset) < 0) {
        print_error(__func__, "sigprocmask() failed: %s\n", strerror(errno));
        return false;
    }

    return true;
}

/*
 * シグナルSIGALRMのブロックを解除
 */
bool unblock_sigalrm()
{
    /* シグナルSIGALRMのブロックを解除 */
    if (sigprocmask(SIG_SETMASK, &old_sigset, NULL) < 0) {
        print_error(__func__, "sigprocmask() failed: %s\n", strerror(errno));
        return false;
    }

    return true;
}

/*
 * インターバルタイマーの準備
 */
bool setup_interval_timer()
{
    struct itimerval timer_val;
    
    /* インターバルタイマーの設定 */
    timer_val.it_interval.tv_sec = 1;
    timer_val.it_interval.tv_usec = 0;
    timer_val.it_value.tv_sec = 1;
    timer_val.it_value.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &timer_val, NULL) < 0) {
        print_error(__func__, "setitimer() failed\n");
        return false;
    }

    return true;
}

