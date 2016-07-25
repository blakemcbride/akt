/*
  APL Key Translation (version 2) for use with GNU APL

  David B. Lamkins <david@lamkins.net>
  July 2016
*/
  
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <locale.h>
#include <string.h>
#include <sys/select.h>
#include <pty.h>
#include <langinfo.h>

#define PACKAGE "akt"
#define VERSION "2.0"

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
  /* !GEN! */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* !GEN! */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* !GEN! */  0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3,
  /* !GEN! */  3, 2, 2, 1, 3, 1, 3, 1, 3, 3, 3, 3, 3, 2, 3, 3,
  /* !GEN! */  3, 3, 2, 3, 3, 3, 0, 0, 0, 3, 3, 3, 3, 0, 0, 3,
  /* !GEN! */  3, 0, 0, 0, 3, 3, 0, 3, 2, 2, 0, 3, 3, 3, 3, 1,
  /* !GEN! */  3, 3, 3, 3, 3, 3, 1, 3, 3, 3, 3, 1, 3, 1, 3, 3,
  /* !GEN! */  3, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0,
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
        "usage: " PACKAGE " CMD [ARGS...]\n"
        "\n"
        PACKAGE "'s input must be a terminal. Your locale must use a\n"
        "UTF-8 encoding.\n"
        "\n"
        "CMD with its ARGS is spawned as a slave to a pty. " PACKAGE "\n"
        "translates Alt+key keystrokes to APL Unicode characters\n"
        "and passes all other keystrokes unaltered.\n"
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

static int ifd = 0;

static struct termios tio_old;

static void
finalize(void) {
  if (tcsetattr(ifd, 0, &tio_old) == -1)
    perror("finalize/tcsetattr");
}

static int master;

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
  conform_window_size();
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
}

static void
initialize(void) {
  struct termios tio_new;

  setlocale(LC_ALL, "");

  if (tcgetattr(ifd, &tio_old) == -1)
    perror("initialize/tcgetattr");
  memcpy(&tio_new, &tio_old, sizeof(struct termios));

  tio_new.c_lflag &= ~ (ICANON | ISIG | ECHO | ECHOCTL);
  tio_new.c_iflag = ICRNL;
  tio_new.c_cc[VMIN] = 1;
  tio_new.c_cc[VTIME] = 0;

  if (tcsetattr(ifd, TCSAFLUSH, &tio_new) == -1)
    perror("initialize/tcsetattr");
}

static int
do_write(int fd, const void *buf, size_t len) {
  int rc = 0;

  if (len > 0) {
    do {
      errno = 0;
      rc = write(fd, buf, len);
    } while (rc == -1 && (errno == EINTR || errno == EAGAIN));
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
process(char input, mapped_t mapped, size_t *len) {
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
master_to_slave() {
  char output;

  if (do_read(master, &output, 1) == -1) {
    if (errno == EIO) /* EIO when child dies */
      return 0;
    else
      perror("master_to_slave/do_read:master");
  }
  if (do_write(STDOUT_FILENO, &output, 1) == -1)
    perror("master_to_slave/do_write:STDOUT_FILENO");
  return 1;
}

static void
slave_to_master() {
  char input;
  mapped_t mapped;
  size_t len;

  if (do_read(STDIN_FILENO, &input, 1) == -1)
    perror("slave_to_master/do_read:STDIN_FILENO");
  else {
    process(input, mapped, &len);
    if (do_write(master, mapped, len) == -1)
      perror("slave_to_master/do_write:master");
  }
}

static int
ioloop() {
  struct timeval timer;
  int rc;
  fd_set read_fd;
  fd_set write_fd;
  fd_set except_fd;

  FD_ZERO(&read_fd);
  FD_ZERO(&write_fd);
  FD_ZERO(&except_fd);
  FD_SET(master, &read_fd);
  FD_SET(STDIN_FILENO, &read_fd);
  timer.tv_sec = 0;
  timer.tv_usec = 20000;
  do {
    errno = 0;
    rc = select(master+1, &read_fd, &write_fd, &except_fd, &timer);
  } while (rc == -1 && errno == EINTR);
  if (rc == 0)
    handle_key_timer();
  else if (rc == -1)
    perror("ioloop/select");
  if (FD_ISSET(master, &read_fd)) {
    if (!master_to_slave())
      return 0;
  }
  if (FD_ISSET(STDIN_FILENO, &read_fd))
    slave_to_master();
  return 1;
}

static void
spawn(char *args[]) {
  pid_t pid = forkpty(&master, NULL, NULL, NULL);
  if (pid < 0) {
    perror("spawn/forkpty");
  }
  else if (pid == 0) {
    /* child */
    if (execvp(args[1], &args[1]) == -1) {
      perror("spawn/execvp");
      exit(1);
      /* NOTREACHED */
    }
  }
  else {
    /* parent */
    initialize();
    set_handlers();
    conform_window_size();

    while (ioloop()) {}

    finalize();
  }
}

int
main(int argc, char *argv[]) {
  if (!isatty(ifd) || argc == 1 || !strcmp(nl_langinfo(CODESET), "UTF-8")) {
    usage();
    /* NOTREACHED */
  }

  spawn(argv);

  exit(0);
}

#endif
