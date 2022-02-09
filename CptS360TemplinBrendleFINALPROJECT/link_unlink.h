#include "globals.h"

void link(char *old_pathname, char *new_pathname);

void unlink(char *old_pathname, char *new_pathname);

void enter_name(MINODE *pmip, int myino, char *myname);

int rmchild(MINODE *pmip, char *dirname);