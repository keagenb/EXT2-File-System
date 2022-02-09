/*
    Our utility functions, most are referenced from Systems Programming in Unix/Linux by KC
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <unistd.h>
#include "globals.h" 


int get_block(int dev, int blk, char *buf)
{
    lseek(dev, (long)blk*BLKSIZE, 0);
    read(dev,buf, BLKSIZE);
}   

int put_block(int dev, int blk, char *buf)
{
    lseek(dev, blk*BLKSIZE, 0);
    write(dev, buf, BLKSIZE);
}   

int tokenize(char *pathname)
{
    // copy pathname into gpath[]; tokenize it into name[0] to name[n-1]
    // Code in Chapter 11.7.2 
    int index = 0;
    char *tempStr;

    tempStr = strtok(pathname, "/");
    
    do
    {
        name[index++] = tempStr; // fills name with tokenized pathname
    }
    while((tempStr = strtok(NULL, "/")) && index < 64); // 64 is default length for pathname
    
    return index; // returns the "size" of name
}

MINODE *iget(int dev, int ino)
{
    // return minode pointer to loaded INODE
    // Code in Chapter 11.7.2
    for(int index = 0; index < NMINODE; index++)
        if(minode[index].refCount > 0 && (minode[index].dev == dev) && (minode[index].ino == ino))
        {
            minode[index].refCount++;
            return (minode + index);
        }

    int index;
    for(index = 0; index < NMINODE; index++)
        if(minode[index].refCount == 0)
        {
            minode[index] = (MINODE){.refCount = 1, .dev = dev, .ino = ino};
            break;
        }

    char buf[BLKSIZE];
    int blk = (ino - 1) / 8 + inode_start, offset = (ino - 1) % 8;
    get_block(dev, blk, buf);
    minode[index].INODE = *((INODE*)buf + offset);
    return minode + index;
}

int iput(MINODE *mip)
{
    // dispose of minode pointed by mip
    // Code in Chapter 11.7.2
    if (--(mip->refCount) == 0 || mip->dirty)
    {
        char buf[BLKSIZE];
        int blk = (mip->ino - 1) / 8 + inode_start, offset = (mip->ino - 1) % 8;
        get_block(mip->dev, blk, buf);
        *((INODE*)buf + offset) = mip->INODE;
        put_block(mip->dev, blk, buf);
    }
}


int search(MINODE *mip, char *name)
{
  // search for name in (DIRECT) data blocks of mip->INODE
  // return its ino
  // Code in Chapter 11.7.2
  char *cp, temp[256], sbuf[BLKSIZE];
  DIR *dp;

  for (int index = 0; index < 12 && mip->INODE.i_block[index]; index++)
  {
    if (mip->INODE.i_block[index] == 0)
    {
        return 0;
    }
    get_block(mip->dev, mip->INODE.i_block[index], sbuf);
    dp = (DIR*) sbuf;
    cp = sbuf;

    while (cp < sbuf + BLKSIZE)
    {
    strncpy(temp, dp->name, dp->name_len);
    temp[dp->name_len] = 0;
    
    if(strcmp(name,temp) == 0)
    {
        return dp->inode;
    }
    cp += dp->rec_len;
    dp = (DIR*)cp;
    }
  }
  return 0;
   // return ino if found; return 0 if NOT
}



int getino(char *pathname)
{ 
    // return ino of pathname
    // Code in Chapter 11.7.2
   
    MINODE *mip = (pathname[0] == '/') ? root : running->cwd; // only syntax that worked for us
    mip->refCount++;
    
    int size = tokenize(pathname);
    //if pathname is a single slash, name[0] will be null
    int ino;
    if(!name[0])
        ino = mip->ino;
    else
        for(int i = 0; i < size; i++)
        {
            ino = search(mip, name[i]);
            if(ino == 0)
                return 0;
            iput(mip);
            mip = iget(mip->dev, ino);
        }
    iput(mip);
    return ino;
}


int findmyname(MINODE *parent, u32 myino, char *myname) 
{
    // WRITE YOUR code here:
    // search parent's data block for myino;
    // copy its name STRING to myname[ ];
    char sbuf[BLKSIZE], temp[256];
    for(int i = 0; i < 12 && parent->INODE.i_block[i]; i++)
    {
        get_block(parent->dev, parent->INODE.i_block[i], sbuf);
        char * cp = sbuf;
        DIR * dp = (DIR*)sbuf;

        while(cp < sbuf + BLKSIZE)
        {
            if(dp->inode == myino)
            {
                strncpy(myname, dp->name, dp->name_len);
                return 1;
            }
            cp += dp->rec_len;
            dp = (DIR*)cp;
        }
    }
    return 0;
}

int findino(MINODE *mip, u32 *myino) // myino = ino of . return ino of ..
{
  // mip->a DIR minode. Write YOUR code to get mino=ino of .
  //                                         return ino of ..
}

// **** INSTEAD OF MOVING TO allocate_deallocate.c WE LEAVE IT IN UTIL.C ****
// From 11.7.4
int tst_bit(char * buf, int bit)
{
    return buf[bit/8] & (1 << (bit%8));
}
// From 11.7.4
void set_bit(char * buf, int bit)
{
    buf[bit/8] |= (1 << (bit%8));
}
// From 11.7.4
void clr_bit(char * buf, int bit)
{
    buf[bit/8] &= ~(1 << (bit%8));
}

// From 11.7.4
int decFreeInodes(int dev)
{
    char buf[BLKSIZE];
    get_block(dev, 1, buf);
    ((SUPER*)buf)->s_free_inodes_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    ((GD*)buf)->bg_free_inodes_count--;
    put_block(dev, 2, buf);
}

int ialloc(int dev)
{
    char buf[BLKSIZE];
    get_block(dev, imap, buf);
    for(int i = 0; i < ninodes; i++)
        if(tst_bit(buf, i) == 0)
        {
            set_bit(buf, i);
            decFreeInodes(dev);
            put_block(dev, imap, buf);
            return i + 1;
        }
    return 0;
}

int balloc(int dev)
{
    char buf[BLKSIZE];
    get_block(dev, bmap, buf);
    for(int i = 0; i < nblocks; i++)
    {
        if(tst_bit(buf, i) == 0)
        {
            set_bit(buf, i);
            decFreeInodes(dev);
            put_block(dev, bmap, buf);
            return i + 1;
        }
    }
        
    return 0;
}

// From pg. 338
int incFreeInodes(int dev)
{
    char buf[BLKSIZE];
    // increment free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    ((SUPER *)buf)->s_free_inodes_count++;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    ((GD *)buf)->bg_free_inodes_count++;
    put_block(dev, 2, buf);
}

// From pg. 338
void idalloc(int dev, int ino)
{
    char buf[BLKSIZE];
    get_block(dev, imap, buf);
    if(ino > ninodes)
    {
        printf("ERROR: inumber %d out of range.\n", ino);
        return;
    }
    get_block(dev,imap,buf);
    clr_bit(buf,ino-1);
    put_block(dev,imap,buf);
    incFreeInodes(dev);
}

// Referenced on pg. 339
void bdalloc(int dev, int bno)
{
    char buf[BLKSIZE];
    if(bno > nblocks)
    {
        printf("ERROR: bnumber %d out of range.\n", bno);
        return;
    }
    get_block(dev,bmap,buf);
    clr_bit(buf,bno-1);
    put_block(dev,bmap,buf);
    incFreeInodes(dev);
}

// From pg. 334
int ideal_length(int name_len)
{
    return 4 * ((8 + name_len + 3) / 4);
}