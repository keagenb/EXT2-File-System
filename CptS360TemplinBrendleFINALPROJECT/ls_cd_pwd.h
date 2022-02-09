
#include "globals.h"

void mychdir(char *pathname);

void ls_file(int ino, char *filename);

void ls_dir(char *dirname);

void ls(char *pathname);

void rpwd(MINODE *wd);

void pwd();