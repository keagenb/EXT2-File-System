#include "rmdir.h"

// From pg. 339
int rm_child(MINODE *pmip, char *dirname)
{
    //rm_child(MINODE *pmip, char *name)
    char *cp, buf[BLKSIZE];
    DIR *dp, *prev;
    int current;


    // (1). Search parent INODE's data block(s) for the entry of name
    for(int i = 0; i < 12 && pmip->INODE.i_block[i]; i++)
    {
        current = 0;

        get_block(dev, pmip->INODE.i_block[i], buf);
        cp = buf;
        dp = (DIR *) buf;
        prev = 0;
        while (cp < buf + BLKSIZE)
        {
        // (2). Delete name entry from parent directory by:
        // if (first and only entry in a data block)
        // else if (LAST entry in data block)
            if (strncmp(dp->name, dirname, dp->name_len) == 0)
            {
                int ideal = ideal_length(dp->name_len);
                // Last entry 
                if(ideal != dp->rec_len)
                {
                    prev->rec_len += dp->rec_len;
                }
                // First and only entry
                else if(prev == NULL && cp + dp->rec_len == buf + 1024)
                {
                    char empty[BLKSIZE] = {0};
                    put_block(dev, pmip->INODE.i_block[i], empty);
                    bdalloc(dev, pmip ->INODE.i_block[i]);
                    // Dec blk by blksize
                    pmip->INODE.i_size -= BLKSIZE;
                    for(int j = i; j < 11; j++)
                    {
                        pmip->INODE.i_block[j] = pmip->INODE.i_block[j+1];
                    }
                    pmip->INODE.i_block[11] = 0;
                }
                else
                {
                    // Neither first and only, or last
                    int removed = dp->rec_len;
                    char *temp = buf;
                    DIR *last = (DIR *)temp;
                    // Last entry
                    while(temp + last->rec_len < buf + BLKSIZE)
                    {
                        temp += last->rec_len;
                        last = (DIR *) temp;
                    }
                    last->rec_len = last->rec_len + removed;

                    
                    memcpy(cp, cp + removed, BLKSIZE - current - removed + 1);
                }
                put_block(dev, pmip->INODE.i_block[i], buf);
                pmip->dirty = 1;
                return 1;
            }
            // Go to next
            cp += dp->rec_len;
            current += dp->rec_len;
            prev = dp;
            dp = (DIR *) cp;
        }
        return 0;
    }

    printf("** ERROR: Does not exist! **\n");
    return 1;
}

// From pg. 336
void rmdir(char *pathname)
{
    // (1). get in-memory INODE of pathname
    //ino = getino(pathname);
    char dirName[64], baseName[64];

    strcpy(dirName, dirname(pathname));
    strcpy(baseName, basename(pathname));

    char *temp = strdup(pathname);
    int ino = getino(temp);
    free(temp);
    if(!ino)
    {
        printf("ERROR: %s does not exist.\n", pathname);
        return;
    }
    //mip = iget(dev, ino);
    MINODE * mip = iget(dev, ino); 
    if(running->uid != mip->INODE.i_uid && running->uid != 0)
    {
        printf("ERROR: You do not have permission to remove %s.\n", pathname);
        iput(mip);
        return;
    }
    // (2). verify INODE is a DIR (by INODE.i_mode field)
    // minode is not BUSY (refCount = 1);
    // verify DIR is empty (traverse data blocks for number of entries = 2);
    if(!S_ISDIR(mip->INODE.i_mode))
    {
        printf("ERROR: %s is not a directory", pathname);
        iput(mip);
        return;
    }
    if(mip->INODE.i_links_count > 2)
    {
        printf("ERROR: Directory %s is not empty.\n", pathname);
        iput(mip);
        return;
    }
    if(mip->INODE.i_links_count == 2)
    {
        char buf[BLKSIZE], * cp;
        DIR *dp;
        get_block(dev, mip->INODE.i_block[0],buf);
        cp = buf;
        dp = (DIR *) buf;
        cp += dp->rec_len;
        dp = (DIR *) cp; // Move to second entry ".."
        if(dp->rec_len != 1012)
        {
            printf("ERROR: Directory %s is not empty.\n", pathname);
            iput(mip);
            return;
        }
    }
    // deallocate its data blocks and inode
    // bdalloc(mip->dev, mip->INODE.i_block[0]);
    // idalloc(mip->dev, mip->ino);
    for(int i = 0; i < 12; i++)
    {
        if (mip->INODE.i_block[i] == 0)
            continue;
        bdalloc(mip->dev,mip->INODE.i_block[i]);
    }
    idalloc(mip->dev, mip->ino);
    iput(mip);
    // get parent's ino and inode
    // pino = findino(); // get pino from ..etnry in INODE._block[0]
    // pmip = iget(mip->dev, pino);
    MINODE * pmip = iget(dev, getino(dirName));
    rmchild(pmip, baseName);

    // (6.) dec parent links_count by 1; mark parent pmip dirty
    // iput(pmip);
    pmip->INODE.i_links_count--;
    pmip->INODE.i_atime = pmip->INODE.i_mtime = time(0L);
    pmip->dirty = 1;
    // iput(pmip)
    iput(pmip);
}