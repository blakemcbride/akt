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
#include <signal.h>
#include <errno.h>

#define PACKAGE "akt"
#define LOCALEDIR "."
#define VERSION "1.0"

static void
usage(void) {
  fputs(PACKAGE " (APL Keyboard Translator) version " VERSION "\n"
        "\n"
        "Use with GNU APL as:\n"
        "  $ " PACKAGE " | apl\n"
        "\n"
        "Use the Alt or Esc key to type APL characters.\n"
        "\n"
        "Type C-space, C-@ or C-` (this may vary with your\n"
        "terminal) three times to terminate the program.\n"
        "\n"
        "When the output is piped, typing any character after\n"
        "the pipe has been broken will terminate the program.\n",
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
  0,   /* . */
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
  "⍀", /* > */
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
  "⌺", /* K */
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
alarm(int signal) {
  const char *t = map['['];
  do_write(t, strlen(t));
  esc = csi_pend = 0;
}

static void
timer_arm(void) {
  struct itimerval timer = { 0 };
  timer.it_value.tv_usec = 50000;
  if (setitimer(ITIMER_REAL, &timer, NULL) == -1)
    perror("timer_arm/setitimer");
}

static void
timer_disarm(void) {
  struct itimerval timer = { 0 };
  if (setitimer(ITIMER_REAL, &timer, NULL) == -1)
    perror("timer_disarm/setitimer");
}

static struct termios old;

static void
initialize(void) {
  struct termios new;

  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);

  if (tcgetattr(ifd, &old) == -1)
    perror("tcgetattr");
  if (tcgetattr(ifd, &new) == -1)
    perror("tcgetattr");

  new.c_lflag &= ~ (ICANON | ISIG | ECHO | ECHOCTL);
  new.c_iflag = ICRNL;
  new.c_cc[VMIN] = 1;
  new.c_cc[VTIME] = 0;

  if (tcsetattr(ifd, TCSAFLUSH, &new) == -1)
    perror("tcgetattr");

  if (signal(SIGALRM, alarm) == SIG_ERR)
    perror("signal");
}

static void
finalize(void) {
  if (tcsetattr(ifd, 0, &old) == -1)
    perror("tcsetattr");
}

int
main(int argc, char *argv[]) {
  if (argc > 1)
    usage();

  initialize();

  int quit = 0;
  while (1) {
    unsigned char buf[16];

    int n = read(ifd, buf, 1);
    if (n == 1) {
      if (esc) {
        if (buf[0] == '[') {
          timer_arm();
          csi_pend = 1;
        }
        else if (csi_pend) {
          timer_disarm();
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
        quit = 0;
      }
      else if (buf[0] == 033) {
        esc = 1;
        quit = 0;
      }
      else if (buf[0] == 0) {
        if (++quit >= 3) break;
      }
      else {
        do_write(buf, 1);
        quit = 0;
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
