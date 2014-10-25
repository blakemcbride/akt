/*
  APL Key Translation for use with GNU APL

  David B. Lamkins <david@lamkins.net>
  October 2014

  Resemblances between some portions of this code and the Linux
  `showkey` code are *not* coincidental.
*/
  
#include <stdio.h>
#include <termios.h>
#include <locale.h>
#include <stdlib.h>
#include <libintl.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <linux/limits.h>

#define PACKAGE "akt"
#define LOCALEDIR "."
#define VERSION "1.2"

static void
usage(void) {
  fputs(PACKAGE " (APL Keyboard Translator) version " VERSION "\n"
        "\n"
        PACKAGE "'s input must be a terminal.\n"
        "\n"
        "You can use " PACKAGE " with any program that accepts UTF-8\n"
        "encoded characters on stdin.\n"
        "\n"
        "Use with GNU APL as:\n"
        "  $ " PACKAGE " | apl\n"
        "\n"
        "Set your terminal emulator to send an ESC prefix when\n"
        "you use the Alt key. This is often described as \"meta\n"
        "sends escape\". Disable Alt key acccess to your terminal\n"
        "emulator's menus. Then hold down the Alt key to type APL\n"
        "characters.\n"
        "\n"
        "Alternatively, some older terminal emulators set bit 7 of\n"
        "a typed character when the Alt key is pressed. " PACKAGE " also\n"
        "translates this input.\n"
        "\n"
        "Some programs disable the SIGINT signal. Invoke " PACKAGE " with\n"
        "the -n option for these programs. For example:\n"
        "  $ " PACKAGE " -n | nano\n"
        ,
        stderr);
  exit(1);
}

static const char* map[128] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* control chars */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* control chars */
  0, /* space */
  "⌶", /* ! */
  "≢", /* " */
  "⍒", /* # */
  "⍋", /* $ */
  "⌽", /* % */
  "⊖", /* & */
  "⍕", /* ' */
  "⍱", /* ( */
  "⍲", /* ) */
  "⍟", /* * */
  "⌹", /* + */
  "⍝", /* , */
  "×", /* - */
  "⍀", /* . */
  "⌿", /* / */
  "∧", /* 0 */
  "¨", /* 1 */
  "¯", /* 2 */
  "<", /* 3 */
  "≤", /* 4 */
  "=", /* 5 */
  "≥", /* 6 */
  ">", /* 7 */
  "≠", /* 8 */
  "∨", /* 9 */
  "≡", /* : */
  "⍎", /* ; */
  "⍪", /* < */
  "÷", /* = */
  "⍙", /* > */
  "⍠", /* ? */
  "⍫", /* @ */
  "⍶", /* A */
  "£", /* B */
  "⍧", /* C */
  "◊", /* D */
  "⍷", /* E */
  "⍫", /* F */
  0,   /* G */
  "⍙", /* H */
  "⍸", /* I */
  "⍤", /* J */
  "⌸", /* K */
  "⌷", /* L */
  0,   /* M */
  0,   /* N */
  "⍥", /* O */
  "⍣", /* P */
  0,   /* Q */
  0,   /* R */
  0,   /* S */
  "⍨", /* T */
  0,   /* U */
  0,   /* V */
  "⍹", /* W */
  "χ", /* X */
  "¥", /* Y */
  0,   /* Z */
  "←", /* [ */
  "⊢", /* \ */
  "→", /* ] */
  "⍉", /* ^ */
  "!", /* _ */
  "◊", /* ` */
  "⍺", /* a */
  "⊥", /* b */
  "∩", /* c */
  "⌊", /* d */
  "∊", /* e */
  "_", /* f */
  "∇", /* g */
  "∆", /* h */
  "⍳", /* i */
  "∘", /* j */
  "'", /* k */
  "⎕", /* l */
  "|", /* m */
  "⊤", /* n */
  "○", /* o */
  "⋆", /* p */
  "?", /* q */
  "⍴", /* r */
  "⌈", /* s */
  "∼", /* t */
  "↓", /* u */
  "∪", /* v */
  "⍵", /* w */
  "⊃", /* x */
  "↑", /* y */
  "⊂", /* z */
  "⍞", /* { */
  "⊣", /* | */
  "⍬", /* } */
  0,   /* ~ */
  0,   /* DEL */
};

static int ifd = 0, ofd = 1;

static void
do_write(const void *buf, size_t len) {
  int rc;
  do {
    errno = 0;
    rc = write(ofd, buf, len);
  } while (rc == -1 && (errno == EINTR || errno == EAGAIN));
  if (rc == -1)
    perror("write");
}

static int esc = 0, csi_pend = 0;

static void
handle_key_timer(int signal) {
  if (csi_pend) {
    const char *t = map['['];
    do_write(t, strlen(t));
  }
  else if (esc)
    do_write("\033", 1);
  esc = csi_pend = 0;
}

static struct termios tio_old;

static void
finalize(void) {
  if (tcsetattr(ifd, 0, &tio_old) == -1)
    perror("tcsetattr");
}

pid_t listener = 0;

static void
find_listener(void) {
  pid_t mypid = getpid();
  const char *pathfmt = "/proc/%u/fd/%u";
  char path[PATH_MAX], slink[PATH_MAX], rlink[PATH_MAX];
  snprintf(path, PATH_MAX, pathfmt, mypid, ofd);
  if (readlink(path, slink, PATH_MAX) == -1)
    perror("readlink:slink");
  if (!strncmp(slink, "pipe:[", 6)) {
    pid_t tpid = mypid;
    FILE *pmfd = fopen("/proc/sys/kernel/pid_max", "r");
    pid_t maxpid = 0;
    fscanf(pmfd, "%u", &maxpid);
    fclose(pmfd);
    int i = 10;
    while (i) {
      if (tpid == maxpid) tpid = 1;
      snprintf(path, PATH_MAX, pathfmt, ++tpid, ifd);
      if (readlink(path, rlink, PATH_MAX) == -1)
        continue;
      if (!strncmp(slink, rlink, PATH_MAX)) break;
      --i;
    }
    if (i) listener = tpid;
  }
}

static void
handle_watchdog_timer(int signal) {
  if (listener) {
    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "/proc/%u", listener);
    struct stat sbuf;
    if (stat(path, &sbuf) == -1) {
      finalize();
      exit(0);
    }
  }
}

static void
key_timer_arm(void) {
  if (signal(SIGALRM, handle_key_timer) == SIG_ERR)
    perror("key_timer_arm/signal");

  struct itimerval timer = { 0 };
  timer.it_value.tv_usec = 50000;
  if (setitimer(ITIMER_REAL, &timer, NULL) == -1)
    perror("key_timer_arm/setitimer");
}

static void
key_timer_disarm(void) {
  struct itimerval timer = { 0 };
  if (setitimer(ITIMER_REAL, &timer, NULL) == -1)
    perror("key_timer_disarm/setitimer:key");

  if (listener) {
    if (signal(SIGALRM, handle_watchdog_timer) == SIG_ERR)
      perror("key_timer_disarm/signal");

    timer.it_value.tv_usec = 200000;
    timer.it_interval.tv_usec = 200000;
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1)
      perror("key_timer_disarm/setitimer:watchdog");
  }
}

static void
initialize(void) {
  struct termios tio_new;

  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);

  if (tcgetattr(ifd, &tio_old) == -1)
    perror("tcgetattr");
  memcpy(&tio_new, &tio_old, sizeof(struct termios));

  tio_new.c_lflag &= ~ (ICANON | ISIG | ECHO | ECHOCTL);
  tio_new.c_iflag = ICRNL;
  tio_new.c_cc[VMIN] = 1;
  tio_new.c_cc[VTIME] = 0;

  if (tcsetattr(ifd, TCSAFLUSH, &tio_new) == -1)
    perror("tcsetattr");
}

int
main(int argc, char *argv[]) {
  int ctrl_c_pass = argc > 1 && !strcmp(argv[1], "-n");

  if ((argc > 1 && !ctrl_c_pass) ||
      !isatty(ifd))
    usage();

  initialize();
  find_listener();
  key_timer_disarm();

  while (1) {
    unsigned char buf[8];
    if (read(ifd, buf, 1) == 1) {
      if (esc) {
        if (buf[0] == '[') {
          csi_pend = 1;
        }
        else if (csi_pend) {
          key_timer_disarm();
          esc = csi_pend = 0;
          do_write("\033[", 2);
          do_write(buf, 1);
        }
        else {
          const char *t = map[buf[0]];
          if (t) {
            do_write(t, strlen(t));
          }
          esc = 0;
        }
      }
      else if (buf[0] & 0x80) {
        const char *t = map[buf[0] & 0x7f];
        if (t) {
          do_write(t, strlen(t));
        }
        esc = csi_pend = 0;
      }
      else {
        switch(buf[0]) {
        case 033: /* ESC */
          key_timer_arm();
          esc = 1;
          break;
        case 3: /* Ctrl-C */
          key_timer_disarm();
          if (ctrl_c_pass)
            do_write(buf, 1);
          else if (listener)
            kill(listener, SIGINT);
          else {
            finalize();
            exit(0);
          }
          break;
        default:
          key_timer_disarm();
          do_write(buf, 1);
        }
      }

      if (!esc)
        tcflush(ofd, TCOFLUSH);
    }
    else
      break;
  }

  finalize();
  exit(0);
}
