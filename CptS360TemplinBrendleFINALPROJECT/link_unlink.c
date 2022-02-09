#include "link_unlink.h"

// From pg. 340
void link(char *old_pathname, char *new_pathname)
{
    // (1). verify old_file exists and is not a DIR
    // oino = getino(old_file);
    int oino = getino(old_pathname);
    if(!oino)
    {
        printf("** ERROR: old_file doesn't exist! **\n");
        return;
    }
    // omip = iget(dev, oino);
    MINODE *omip = iget(dev, oino);
    // check omip->INODE file type (must not be DIR)
    if(S_ISDIR(omip->INODE.i_mode))
    {
        printf("** ERROR: Cannot link/unlink directories! **\n");
        return;
    }

    // (2). new_file must not exist yet
    // getino(new_file) must return 0;
    // (3). creat new_file with the same inode number of old_file
    // parent  dirname(new_file); child = basename(new_file);
    // creat entry in new parent DIR with same inode number of old_file
    char dirName[64], baseName[64];

    strcpy(dirName, dirname(new_pathname));
    strcpy(baseName, basename(new_pathname));

     // pino = getino(parent)
    int pino = getino(dirName);
    if(!pino)
    {
        printf("** ERROR: new_file doesn't exist! **\n");
        return;
    }
    // pmip = iget(dev, ino)
    MINODE *pmip = iget(dev, pino);
    if(!S_ISDIR(pmip->INODE.i_mode))
    {
        printf("** ERROR: Not a directory. **\n");
        return;
    }
    char *base = strdup(baseName);
    if(search(pmip, base))
    {
        printf("** ERROR: new_file already exists! **\n");
        return;
    }
    free(base);
    // enter_name(pmip, oino, child);
    enter_name(pmip, oino, baseName);

    // (4). omip->INODE.i_links_count++;
    // inc INODE's links_count by 1
    omip->INODE.i_links_count++;
    // omip->dirty = 1; for write back by iput(omip)
    omip->dirty = 1;

    // iput(omip)
    iput(omip);
    // iput(pmip);
    iput(pmip);
}

// From pg. 342
void unlink(char *old_pathname, char *new_pathname)
{
    // (1). get filename's minode
    // ino = getino(filename)
    int ino = getino(old_pathname);
    if(!ino)
    {
        printf("** ERROR: Cannnot find specified path! **\n");
        return;
    }
    // mip = iget(dev, ino);
    MINODE *mip = iget(dev, ino);
    // check it's a REG or symbolic LNK file; cannot be a DIR
    if(S_ISDIR(mip->INODE.i_mode))
    {
        printf("** ERROR: Cannot link/unlink directories! **\n");
        return;
    }
    // (2). remove name entry from parents DIR's data block
    // (3). dec INODE's link_count by 1
    // mip->INODE.i_links_count--;
    if(mip->INODE.i_links_count == 0)
    {
        idalloc(mip->dev, mip->ino);
    }
    // (4). if(mip->INODE.i_links_count > 0)
    //  mip->dirty = 1; for write INDOE back to disk
    else
    {
        mip->dirty = 1;
    }

    // parent = dirname(filename); child = basename(filename);
    // pino = getino(parent);
    // pmip = iget(dev, pino);
    char dirName[64], baseName[64];

    strcpy(dirName, dirname(new_pathname));
    strcpy(baseName, basename(new_pathname));

    MINODE * parent = iget(dev, getino(dirName));
    rmchild(parent, baseName);

    iput(parent);
    // iput(mip) // release mip
    iput(mip);
}

// From pg. 334
void enter_name(MINODE *pmip, int myino, char *myname)
{
    // for each data block of parent DIR do // assume: only 12 direct blocks
    int need_len = ideal_length(strlen(myname));
    for(int i = 0; i < 12; i++)
    {
        char buf[BLKSIZE];
        DIR * dp = (DIR*)buf;
        char * cp = buf;

        if(pmip->INODE.i_block[i] == 0)
        {
            // (1). Get parent's data block into a buf[];
            pmip->INODE.i_block[i] = balloc(dev);
            get_block(pmip->dev, pmip->INODE.i_block[i], buf);
            *dp = (DIR){.inode = myino, .rec_len = BLKSIZE, .name_len = strlen(myname)};
            
            strncpy(dp->name, myname, dp->name_len);
            put_block(pmip->dev, pmip->INODE.i_block[i], buf);
            return;
        }
 
        get_block(pmip->dev, pmip->INODE.i_block[i], buf);

        //get to the last entry in the block
        while(cp + dp->rec_len < buf + BLKSIZE)
        {
            cp += dp->rec_len;
            dp = (DIR*)cp;
        }
        // remain = LAST entry's rec_len - its ideal_length
        int remain = dp->rec_len - ideal_length(dp->name_len);
        // if (remain >= need_length)
        if(remain >= need_len)
        {
            // In a data block of the parent directory, each entry has an ideal length
            // All dir_entrie rec_len = ideal_length except the last entry
            dp->rec_len = ideal_length(dp->name_len);
            cp += dp->rec_len;
            dp = (DIR*)cp;
            dp->inode = myino;
            dp->rec_len = remain;
            dp->name_len = strlen(myname);
            strncpy(dp->name, myname, dp->name_len);
            put_block(pmip->dev, pmip->INODE.i_block[i], buf);
            pmip->dirty = 1;
            return;
        }
    }
}

// Brought function from rmdir.c for ease
// From pg. 339
int rmchild(MINODE *pmip, char *dirname)
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
                if(ideal != dp->rec_len) // Last entry 
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
                    // last entry
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