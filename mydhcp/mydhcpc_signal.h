
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcpc_signal.h */

#ifndef MYDHCPC_SIGNAL_H
#define MYDHCPC_SIGNAL_H

#include <signal.h>
#include <stdbool.h>

/*
 * シグナルSIGALRMが発生したかどうかのフラグ
 */
extern volatile sig_atomic_t is_sigalrm_handled;

/*
 * シグナルSIGHUPが発生したかどうかのフラグ
 */
extern volatile sig_atomic_t app_exit;

/*
 * シグナルSIGALRMのハンドラ
 */
void sigalrm_handler(int sig);

/*
 * シグナルSIGHUPのハンドラ
 */
void sighup_handler(int sig);

/*
 * シグナルハンドラを設定
 */
bool setup_signal_handlers();

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

#endif /* MYDHCPC_SIGNAL_H */

