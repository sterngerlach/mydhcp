
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcps_signal.h */

#ifndef MYDHCPS_SIGNAL_H
#define MYDHCPS_SIGNAL_H

#include <signal.h>
#include <stdbool.h>

/*
 * シグナルSIGALRMが発生したかどうかのフラグ
 */
extern volatile sig_atomic_t is_sigalrm_handled;

/*
 * シグナルSIGALRMのハンドラ
 */
void sigalrm_handler(int sig);

/*
 * シグナルSIGALRMのハンドラを設定
 */
bool setup_sigalrm_handler();

/*
 * シグナルSIGALRMをブロック
 */
bool block_sigalrm();

/*
 * シグナルSIGALRMのブロックを解除
 */
bool unblock_sigalrm();

/*
 * インターバルタイマーの準備
 */
bool setup_interval_timer();

#endif /* MYDHCPS_SIGNAL_H */

