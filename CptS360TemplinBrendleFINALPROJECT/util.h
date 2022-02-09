/*
    For simplicity sake we created a header file for our utility functions
*/
#pragma once

#include "type.h"

int get_block(int dev, int blk, char * buf);
int put_block(int dev, int blk, char * buf);
int tokenize(char * pathname);
MINODE * iget(int dev, int ino);
int iput(MINODE * mip);
int search(MINODE * mip, char * name);
int getino(char * pathname);
int findmyname(MINODE * parent, u32 myino, char * myname);
int findino(MINODE *mip, u32 *myino);
int tst_bit(char * buf, int bit);
void set_bit(char * buf, int bit);
void clr_bit(char * but, int bit);
int decFreeInodes(int dev);
int incFreeInodes(int dev);
int ialloc(int dev);
int balloc(int dev);
void idalloc(int dev, int ino);
void bdalloc(int dev, int bno);