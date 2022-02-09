// symbolic link files (as outline in pg. 3 of "FP_CptS360_Spring2021.docx")

#include "symlink_readlink.h"

// From pg. 342
void symlink(char *old_pathname, char *new_pathname)
{
    // (1). check old_file must exist and new_file must not yet
    char *pathname_dup = strdup(old_pathname);
    if(!getino(pathname_dup))
    {
        free(pathname_dup);
        printf("** ERROR: old_file does not exist! **\n");
        return;
    }
    free(pathname_dup);
    // (2). creat new_file and change it to LNK type
    int ino = make(0, new_pathname);
    MINODE *mip = iget(dev, ino);
    mip->INODE.i_mode = 0xA1A4;
    // (3). store old_file name in newfile's INODE.i_block[] area
    strcpy((char*)(mip->INODE.i_block), old_pathname);
    // (4). iput(new_file's parent minode)
    iput(mip);
}

// From pg. 343
void readlink(char *pathname)
{
    // (1). get file's INODE in memory
    int ino = getino(pathname);
    if(!ino)
    {
        printf("** ERROR: pathname does not exist! **\n");
        return;
    }
    MINODE *mip = iget(dev, ino);
    // verify it's a LNK file
    if(!S_ISLNK(mip->INODE.i_mode))
    {
        printf("** ERROR: Desired file is not a link file! **\n");
        return;
    }
    // (2). copy target filename from INODE.i_block[]
    printf("%s\n", (char*)(mip->INODE.i_block));
    
    iput(mip);
}