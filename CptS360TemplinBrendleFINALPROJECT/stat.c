// return file information (as outline in pg. 3 of "FP_CptS360_Spring2021.docx")

#include "stat.h"

// Modeled after the stat format that gets printed in the sample a.out solution
void mystat(char *pathname)
{
    // Get file's INODE in memory
    int ino = getino(pathname);
    if(!ino)
    {
        printf("File not found\n");
        return;
    }
    MINODE *mip = iget(dev, ino);
    
    // Accessing file name with basename command
    char baseName[64];
    strcpy(baseName, basename(pathname));
    printf("File: %s\n", baseName);

    // Using i_mode to determind file type
    char *file;
    if(S_ISDIR(mip->INODE.i_mode))
    {
        file = "DIR";
    }
    else if(S_ISLNK(mip->INODE.i_mode))
    {
        file = "LNK";
    }
    else
    {
        file = "REG";
    }

    // Accessing file's time data
    time_t mod = (time_t) mip->INODE.i_atime;

    // Printing everything to screen (made to look similar to the sample a.out's stat printout)
    printf("\n************ STAT ************\n");
    printf("Dev: %d   ", mip->dev);
    printf("Inode: %d   ", ino);
    printf("Type: %s\n", file);
    printf("UID: %d   ", mip->INODE.i_uid);
    printf("GID: %d   ", mip->INODE.i_gid);
    printf("   NLink: %d\n", mip->INODE.i_links_count);
    printf("Time: %s", ctime(&mod));
    printf("Size: %d   ", mip->INODE.i_size);
    printf("Blocks: %d\n", mip->INODE.i_blocks);
    printf("******************************\n\n");

    iput(mip);
}