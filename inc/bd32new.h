/* bd32new.h - main BD32 debugger header */

#pragma once

#include "scrninfo.h"
#include "dasmdefs.h"

/* Control break handling */
int HandleCtrlBreak(void);

/* Window management */
void SetTextInfo(struct text_info *TheInfo);
void GetWindow(struct text_info *Window);
void SetWindow(struct text_info *Window);
void RepeatChar(char Char, unsigned Count, unsigned char Attribute);
int SetScroll(int NewValue);
unsigned GetWindowHeight(struct text_info *TheWindow);
unsigned GetWindowWidth(struct text_info *TheWindow);
void CenterString(char *String, unsigned Y);

/* Screen save/restore */
struct ScreenInfo *SaveScreen(void);
void RestoreScreen(struct ScreenInfo *TheInfo);
void SetUpScreen(int FirstTimeThrough);
void SelectLargeCommandLineWindow(void);
void SelectSmallCommandLineWindow(void);

/* Logging functions */
void LogPutch(int c);
void LogPuts(char *string);
void OutputLogBuffer(void);
void CloseLog(void);
int do_log(int argc, char **argv);

/* Output and pause control */
int GetPauseKey(void);
int pause_check(void);
int pf(char *fmt, ...);
void _show_stat(char *workbuf);
void beep(void);
void show_stat(int force);

/* Character handling */
void HandleColumn(char c);
void backspace(void);
void tab(void);
void ot(char c);
void pc(char c);

/* Utility functions */
int cmd_size(char *cmd);
int ntoh(BYTE n);
void btoh(LONG word);
void wtoh(LONG word);
void ltoh(LONG word);
char *skipwhite(char **s);
int htoc(char c);
int gethex(char *where, LONG *data);
void print(char *s);
char *set_opt(char *string);
void parsecmd(char *string, int *count, char **ptrs, int maxparam);

/* Initialization and configuration */
void init(int argc, char **argv);
void read_config(void);
void SetUpPort(void);
int quit(int argc, char **argv);

/* Error handling */
void show_error(char *string);
void report_error(int type, char *string);
void closemacro(void);

/* Command execution */
int execute(char *line);

/* Memory operations */
void pc_getstring(char *dest, LONG src);
void pc_putstring(LONG dest, char *src);
void pc_getbuff(char *dest, LONG src, size_t count);
void pc_putbuff(LONG dest, char *src, size_t count);
void send_srec(srecord *data, LONG address);

/* System calls and program control */
LONG SysCall(int *quitflag);
LONG do_prog(LONG address);
int do_driver(int argc, char **argv);
int do_syscall(int argc, char **argv);
int do_cload(int argc, char **argv);
int ipd(int argc, char **argv, char *extension, int run);

/* Commands */
int do_cls(int argc, char **argv);
int do_go(int argc, char **argv);
void CollectArguments(char *cmd, int argc, char **argv);
int RepeatUntilTrue(int (*function)(int, char **), int argc, char **argv);
void TraceRegisterDisplay(void);
int do_trace(int argc, char **argv);
int OpSkipCount(LONG Address);
int do_step(int argc, char **argv);
int port_usage(void);
int do_port(int argc, char **argv);
int do_stop(int argc, char **argv);
int do_restart(int argc, char **argv);
int do_reset(int argc, char **argv);
int do_memory(int argc, char **argv);

/* Input handling */
int key_ready(void);
int gc(int update);
int check_esc(void);

/* Debug commands */
int do_dump(int argc, char **argv);
int do_bf(int argc, char **argv);
int clear_brkpnts(void);
int setbr(char *stuff, char *mask);
