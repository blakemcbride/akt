/*
  APL Key Translation (version 2) for use with GNU APL

  David B. Lamkins <david@lamkins.net>
  July 2016
*/

#include <errno.h>
#include <langinfo.h>
#include <locale.h>
#include <pty.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>

#define DEBUG 0
#define PACKAGE "akt"
#define VERSION "2.1"

static const char* map[128] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* control chars */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* control chars */
  0,   /*   */ "⌶", /* ! */ "≢", /* " */ "⍒", /* # */
  "⍋", /* $ */ "⌽", /* % */ "⊖", /* & */ "⍕", /* ' */
  "⍱", /* ( */ "⍲", /* ) */ "⍟", /* * */ "⌹", /* + */
  "⍝", /* , */ "×", /* - */ "⍀", /* . */ "⌿", /* / */
  "∧", /* 0 */ "¨", /* 1 */ "¯", /* 2 */ "<", /* 3 */
  "≤", /* 4 */ "=", /* 5 */ "≥", /* 6 */ ">", /* 7 */
  "≠", /* 8 */ "∨", /* 9 */ "≡", /* : */ "⍎", /* ; */
  "⍪", /* < */ "÷", /* = */ "⍙", /* > */ "⍠", /* ? */
  "⍫", /* @ */ "⍶", /* A */ "£", /* B */ "⍧", /* C */
  "◊", /* D */ "⍷", /* E */ 0,   /* F */ 0,   /* G */
  0,   /* H */ "⍸", /* I */ "⍤", /* J */ "⌸", /* K */
  "⌷", /* L */ 0,   /* M */ 0,   /* N */ "⍥", /* O */
  "⍣", /* P */ 0,   /* Q */ 0,   /* R */ 0,   /* S */
  "⍨", /* T */ "€", /* U */ 0,   /* V */ "⍹", /* W */
  "χ", /* X */ "¥", /* Y */ 0,   /* Z */ "←", /* [ */
  "⊢", /* \ */ "→", /* ] */ "⍉", /* ^ */ "!", /* _ */
  "◊", /* ` */ "⍺", /* a */ "⊥", /* b */ "∩", /* c */
  "⌊", /* d */ "∊", /* e */ "_", /* f */ "∇", /* g */
  "∆", /* h */ "⍳", /* i */ "∘", /* j */ "'", /* k */
  "⎕", /* l */ "|", /* m */ "⊤", /* n */ "○", /* o */
  "⋆", /* p */ "?", /* q */ "⍴", /* r */ "⌈", /* s */
  "∼", /* t */ "↓", /* u */ "∪", /* v */ "⍵", /* w */
  "⊃", /* x */ "↑", /* y */ "⊂", /* z */ "⍞", /* { */
  "⊣", /* | */ "⍬", /* } */ 0,   /* ~ */ 0,   /* DEL */
};

static const size_t oc[128] = {
  /* OCTBL-BEGIN -- generated; do not edit! */
  /* !GEN! */  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
  /* !GEN! */  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
  /* !GEN! */  0, 3, 3, 3,  3, 3, 3, 3,  3, 3, 3, 3,  3, 2, 3, 3,
  /* !GEN! */  3, 2, 2, 1,  3, 1, 3, 1,  3, 3, 3, 3,  3, 2, 3, 3,
  /* !GEN! */  3, 3, 2, 3,  3, 3, 0, 0,  0, 3, 3, 3,  3, 0, 0, 3,
  /* !GEN! */  3, 0, 0, 0,  3, 3, 0, 3,  2, 2, 0, 3,  3, 3, 3, 1,
  /* !GEN! */  3, 3, 3, 3,  3, 3, 1, 3,  3, 3, 3, 1,  3, 1, 3, 3,
  /* !GEN! */  3, 1, 3, 3,  3, 3, 3, 3,  3, 3, 3, 3,  3, 3, 0, 0,
};

#define MAX_UTF8_OCTETS 4 /* RFC 3269 */

#ifdef GENOCTBL

int
main(int argc, char *argv[]) {
  int i;
  const char *c;
  size_t l;
  int rc = 0;

  for (i = 0; i < 128; ++i) {
    c = map[i];
    if (i%16 == 0) printf("  /* !%s! */ ", "GEN");
    l = c ? strlen(c) : 0;
    if (l > MAX_UTF8_OCTETS) {
      fprintf(stderr, "error at index %d\n", i);
      rc = 1;
    }
    printf(" %ld,", l);
    if (i%4 == 3 && i%16 != 15) putchar(' ');
    if (i%16 == 15) putchar('\n');
  }

  exit(rc);
}

#else

static const char* info="#: APL Keyboard Translator";

static void
usage(void) {
  const char* ignore = info;
  fputs(PACKAGE " (APL Keyboard Translator) version " VERSION "\n"
        "\n"
        "usage: " PACKAGE " [-z] CMD [ARGS...]\n"
        "\n"
        PACKAGE "'s input must be a terminal. Your locale must use a\n"
        "UTF-8 encoding.\n"
        "\n"
        "CMD with its ARGS is spawned as a slave to a pty. " PACKAGE "\n"
        "translates Alt+key keystrokes to APL Unicode characters\n"
        "and passes all other keystrokes unaltered.\n"
        "\n"
        "Use the -z option to suppress the suspend character.\n"
        "\n"
        "Use with GNU APL as:\n"
        "  $ " PACKAGE " apl ...\n"
        "\n"
        "Set your terminal emulator to send an ESC prefix when\n"
        "you use the Alt key. This is often described as \"meta\n"
        "sends escape\". Disable Alt key acccess to your terminal\n"
        "emulator's menus.\n"
        ,
        stderr);
  exit(1);
  /* NOTREACHED */
}

static struct termios tio_old;

static void
initialize(void) {
  struct termios tio_new;

  setlocale(LC_ALL, "");

  if (tcgetattr(STDIN_FILENO, &tio_old) == -1)
    perror("initialize/tcgetattr");
  memcpy(&tio_new, &tio_old, sizeof(struct termios));

  tio_new.c_lflag &= ~ (ICANON | ISIG | ECHO | ECHOCTL);
  tio_new.c_iflag |= IGNBRK;
  tio_new.c_cc[VMIN] = 1;
  tio_new.c_cc[VTIME] = 0;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &tio_new) == -1)
    perror("initialize/tcsetattr");
}

static void
finalize(void) {
  if (tcsetattr(STDIN_FILENO, 0, &tio_old) == -1)
    perror("finalize/tcsetattr");
}

static int master;

static int do_winch = 0;

static void
conform_window_size() {
  struct winsize ws;
  if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1)
    perror("conform_window_size/ioctl:TIOCGWINSZ");
  if (ioctl(master, TIOCSWINSZ, &ws) == -1)
    perror("conform_window_size/ioctl:TIOCSWINSZ");
}

static void
winch_handler(int signal) {
  do_winch = 1;
}

static pid_t child = 0;

static void
suspend() {
  int status;

  if (waitpid(child, &status, WNOHANG|WUNTRACED) == -1)
    perror("suspend:waitpid");
  if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP) {
    finalize();
    if (raise(SIGSTOP) == -1)
      perror("suspend:raise");
  }
}

static void
resume() {
  initialize();
  if (kill(child, SIGCONT) == -1)
    perror("resume:kill");
}

static int do_chld = 0;

static void
chld_handler(int signal) {
  do_chld = 1;
}

static int do_cont = 0;

static void
cont_handler(int signal) {
  do_cont = 1;
}

static void
set_handlers() {
  struct sigaction handler;

  if (sigfillset(&handler.sa_mask) == -1)
    perror("set_handlers/sigfillset");
  handler.sa_handler = winch_handler;
  handler.sa_flags = 0;
  if (sigaction(SIGWINCH, &handler, 0) == -1)
    perror("set_handlers/sigaction:SIGWINCH");
  handler.sa_handler = chld_handler;
  if (sigaction(SIGCHLD, &handler, 0) == -1)
    perror("set_handlers/sigaction:SIGCHLD");
  handler.sa_handler = cont_handler;
  if (sigaction(SIGCONT, &handler, 0) == -1)
    perror("set_handlers/sigaction:SIGCONT");
}

static int
do_write(int fd, const void *buf, size_t len) {
  int rc = 0;
   FILE *fp1;

  if (len > 0) {
    do {
      errno = 0;
      rc = write(fd, buf, len);
    } while (rc == -1 && (errno == EINTR || errno == EAGAIN));
#if DEBUG
    if (rc == -1) {
       fp1=fopen("log.txt", "a");
       fprintf(fp1, "errno = %d\n", errno);
       fclose(fp1);
    }
#endif
  }

  return rc;
}

static int
do_read(int fd, void *buf, size_t len) {
   int rc;

   do {
     errno = 0;
     rc = read(fd, buf, len);
   } while (rc == -1 && errno == EINTR);

   return rc;
}

typedef enum { s_pass, s_esc, s_csi, s_O } state_t;
static state_t state = s_pass;

typedef char mapped_t[MAX_UTF8_OCTETS+1];

static char ESC = 033;

static void
next(state_t follow, size_t *len) {
  state = follow;
  *len = 0;
}

static void
emit(char input, mapped_t mapped, size_t *len) {
  mapped[0] = input;
  *len = 1;
}

static void
flush_prefix(state_t next) {
  const char *t;
  size_t l;

  if (state == s_csi) {
    t = map['['];
    l = oc['['];
  }
  else if (state == s_O) {
    t = map['O'];
    l = oc['O'];
  }
  else if (state == s_esc) {
    t = &ESC;
    l = 1;
  }
  else
    return;
  do_write(master, t, l);
  state = next;
}

static void
pass_xlate(char input, mapped_t mapped, size_t *len) {
  const char *t;

  t = map[input];
  if (t) {
    memcpy(mapped, t, *len = oc[input]);
  }
  else
    *len = 0;
  state = s_pass;
}

static void
pass_seq(char c1, char c2, mapped_t mapped, size_t *len) {
  mapped[0] = ESC;
  mapped[1] = c1;
  mapped[2] = c2;
  *len = 3;
  state = s_pass;
}

static void
process_key(char input, mapped_t mapped, size_t *len) {
  if (input&0x80)
    emit(input, mapped, len);
  else if (state == s_pass) {
    if (input == ESC)
      next(s_esc, len);
    else
      emit(input, mapped, len);
  }
  else {
    if (state == s_esc) {
      if (input == '[')
        next(s_csi, len);
      else if (input == 'O')
        next(s_O, len);
      else
        pass_xlate(input, mapped, len);
    }
    else if (state == s_csi) {
      if (input == ESC)
        flush_prefix(s_esc);
      else
        pass_seq('[', input, mapped, len);
    }
    else if (state == s_O) {
      if (input == ESC)
        flush_prefix(s_esc);
      else
        pass_seq('O', input, mapped, len);
    }
    else
      pass_xlate(input, mapped, len);
  }
}

static void
handle_key_timer() {
  flush_prefix(s_pass);
}

static int
master_to_client() {
  char output;

  if (do_read(master, &output, 1) == -1) {
    if ((errno == EIO) || (errno == EPIPE)) /* EIO when child dies */
      return 0;
    else
      perror("master_to_slave/do_read:master");
  }
  if (do_write(STDOUT_FILENO, &output, 1) == -1)
    perror("master_to_slave/do_write:STDOUT_FILENO");
  return 1;
}

static int nosuspend = 0;

static void
client_to_master() {
  char input;
  mapped_t mapped;
  size_t len;
  int rc=0;

  if (do_read(STDIN_FILENO, &input, 1) == -1)
    perror("slave_to_master/do_read:STDIN_FILENO");
  else {
    if (input == tio_old.c_cc[VSUSP] && nosuspend)
      return;
    process_key(input, mapped, &len);
    rc=do_write(master, mapped, len);
    if ((rc == -1) && (errno != EIO))
      perror("slave_to_master/do_write:master");
    if (errno == EIO)
      exit(0);
  }
}

static void
process_signals() {
  if (do_winch) {
    conform_window_size();
    do_winch = 0;
  }
  if (do_chld) {
    suspend();
    do_chld = 0;
  }
  if (do_cont) {
    resume();
    do_cont = 0;
  }
}

static int
ioloop() {
  struct timeval timer;
  int rc;
  fd_set read_fd;

  FD_ZERO(&read_fd);
  FD_SET(master, &read_fd);
  FD_SET(STDIN_FILENO, &read_fd);
  timer.tv_sec = 0;
  timer.tv_usec = 20000;
  do {
    errno = 0;
    rc = select(master+1, &read_fd, NULL, NULL, &timer);
  } while (rc == -1 && errno == EINTR);
  if (rc == 0)
    handle_key_timer();
  else if (rc == -1)
    perror("ioloop/select");
  process_signals();
  if (FD_ISSET(master, &read_fd)) {
    if (!master_to_client())
      return 0;
  }
  if (FD_ISSET(STDIN_FILENO, &read_fd))
    client_to_master();
  return 1;
}

static void
spawn(char *args[]) {
  pid_t pid = forkpty(&master, NULL, NULL, NULL);
  if (pid < 0) {
    perror("spawn/forkpty");
  }
  else if (pid == 0) {
    /* child = server */
    if (execvp(args[0], &args[0]) == -1) {
      perror("spawn/execvp");
      exit(1);
      /* NOTREACHED */
    }
  }
  else {
    /* parent = client */
    child = pid;
    initialize();
    set_handlers();
    conform_window_size();

    while (ioloop()) {}

    finalize();
  }
}

int
main(int argc, char *argv[]) {
  int opt;

  if (!isatty(STDIN_FILENO)
      || argc == 1
      || !strcmp(nl_langinfo(CODESET), "UTF-8")) {
    usage();
    /* NOTREACHED */
  }

  while ((opt = getopt(argc, argv, "+z")) != -1) {
    switch (opt) {
    case 'z':
      nosuspend = 1;
      break;
    default:
      usage();
      /* NOTREACHED */
    }
  }

  spawn(&argv[optind]);

  exit(0);
}

#endif
