#include "misc1.h"

void chmodNEW(char *pathname, char *pathExtra)
{
    int ino = getino(pathname);

    if (ino)
    {
        MINODE *mip = iget(dev, ino); 

        int newMode;
        sscanf(pathExtra, "%o", &newMode); // scan for permission input
        
        int lastMode = mip->INODE.i_mode;
        lastMode >>= 9;
        newMode = newMode | (lastMode << 9);

        mip->INODE.i_mode = newMode;
        mip->dirty = 1;

    iput(mip);
    }
    else // no inode for this file
    {
        printf("** ERROR: This file does not exist! **\n");
        return;
    }
}