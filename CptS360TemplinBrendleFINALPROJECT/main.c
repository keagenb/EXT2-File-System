/****************************************************************************
*                   KCW testing ext2 file system                            *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "ls_cd_pwd.h"
#include "link_unlink.h"
#include "stat.h"
#include "misc1.h"
#include "mkdir_creat.h"
#include "symlink_readlink.h"
#include "rmdir.h"
#include "type.h"
#include "util.h"

// globals
MINODE minode[NMINODE];
MINODE *root;

PROC proc[NPROC], *running;
OFT mtable[NFD];

char gpath[128];    // global for tokenized components
char *name[32];     // assume at most 32 components in pathname
int  n;             // number of component strings

int  fd, dev;
int  nblocks, ninodes, bmap, imap, inode_start, iblock;

//                  0       1       2       3      4       5        6       7       8           9           10      11        12        13      14      15
char *cmds[] = {"mkdir", "creat", "ls", "cd", "pwd", "rmdir", "link", "unlink", "symlink", "readlink", "stat", "access", "chmod", "chown", "utime", "quit"}; // instead of if statements, we use a switch statement to call our functions
char *rootdev = "disk1"; // default disk for now, will be changed by the user

int fs_init() // based off of the books implementation
{
    int i,j;
    for (i = 0; i < NMINODE; i++) // initialize minodes
    {
        minode[i].refCount = 0;
    }
    for (i = 0; i < NFD; i++) // initialize mtable
    {
       // mtable[i].mptr->dev = 0;
    }
    for (i = 0; i < NPROC; i++)
    {
        proc[i].status = READY;
        proc[i].pid = i;
        proc[i].uid = i;

        for (j = 0; j < NFD; j++)
        {
            proc[i].fd[j] = 0;
            proc[i].next = &proc[i+1];
        }
        proc[NPROC-1].next = &proc[0];
        running = &proc[0];
    }
}

int mount_root(char *rootdev) // opens up the device and ensures that it is an EXT2 File system
{
    int i;
    OFT *mp;
    SUPER *sp;
    GD *gp;
    char buf[BLKSIZE];
    
    dev = open(rootdev, O_RDWR);

    if (dev < 0)
    {
        printf("ERROR: Can't open root device!\n");
        exit(1);
    }
    // get the superblock
    get_block(dev, 1, buf);
    sp = (SUPER*)buf;
    // check magic number
    if (sp->s_magic != EXT2_SUPER_MAGIC)
    {
        printf("SUPER MAGIC: %x, %s is not an EXT2 filesystem!\n", sp->s_magic, rootdev);
        exit(0);
    }
    // fill mount table mtable[0] with rootdev info
    mp = &mtable[0]; 
   
    //mp->mptr->dev = dev; // ERROR 
    // copy super block info into mtable[0]
    ninodes = sp->s_inodes_count;
    nblocks = sp->s_blocks_count;

    //strcpy(mp->mptr->dev, rootdev);
    //strcpy(mp->mptr->mounted, "/");

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    bmap = gp->bg_block_bitmap;
    imap = gp->bg_inode_bitmap;
    iblock = gp->bg_inode_table;
    inode_start = gp->bg_inode_table;

    printf("bmap = %d, imap = %d, iblock = %d\n", bmap, imap, iblock);

    // call iget()
    root = iget(dev, 2);
   // mp->mptr = root;
  //  root->mptr = mp;
    // set proc CWDs
    for (i = 0; i < NPROC; i++)
    {
        proc[i].cwd = iget(dev, 2);
    }

    printf("Mount: %s mounted on / \n", rootdev);
    printf("\n** Device Info **\n");
    printf("\nnblocks = %d | ninodes = %d\n", nblocks, ninodes);
    return 0;
}

void quit()
{
    for (int i = 0; i < NMINODE; i++)
        if (minode[i].refCount > 0 && minode[i].dirty == 1)
        {
            minode[i].refCount = 1;
            iput(&(minode[i]));
        }
    exit(0); 
}

int main(int argc, char * argv[])
{
    int index;
    char line[128], cmd[16], pathname[64], pathExtra[64]; // pathExtra is used to read in triad inputs

    if (argc > 1)
    {
       rootdev = argv[1];
    }
    else
    {
        printf("\n** Run the output file with 'diskname' at the end **\n");
        exit(1);
    }

    fs_init();
    mount_root(rootdev);

    while(1)
    {
        // resets the parameters so ls does not act as cd
        pathname[0] = '\0';
        cmd[0] = '\0';

        printf("Process ID: %d\n", running->pid);
        printf("Enter Command: \n");
        fgets(line, 128, stdin);
        line[strlen(line)-1] = 0;

        if (line[0] == 0)
        {
            continue;
        }
        
       sscanf(line, "%s %s %s", cmd, pathname, pathExtra);
        
        index = findCmd(cmd);
        
        switch(index) // these function calls are from other files (such as ls_cd_pwd.h)
        {
            case 0: makedir(pathname);
            break;
            case 1: create_file(pathname);
            break;
            case 2: ls(pathname);
            break;
            case 3: mychdir(pathname);
            break;
            case 4: pwd();
            break;
            case 5: rmdir(pathname);
            break;
            case 6: link(pathname, pathExtra);
            break;
            case 7: unlink(pathname, pathExtra);
            break;
            case 8: symlink(pathname, pathExtra);
            break;
            case 9: readlink(pathname);
            break;
            case 10: mystat(pathname);
            break;
            case 11: // access
            break;
            case 12: chmodNEW(pathname, pathExtra);
            break;
            case 13: // chown
            break;
            case 14: // utime
            break;
            case 15: quit();
            break;
            default: printf("** ERROR: Not a valid command. **\n");
        }
    }
}

int findCmd(char *cmmd) // this is needed to find the cmd (similar to lab 10 implementation)
{
   int i = 0;
   while(cmds[i]){
     if (strcmp(cmmd, cmds[i])==0)
         return i;
     i++;
   }
   return -1;
}