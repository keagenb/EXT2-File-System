#include "mkdir_creat.h"

// From pg. 336
int make(int dir, char * name)
{
    char dirName[64], baseName[64];

    strcpy(dirName, dirname(name));
    strcpy(baseName, basename(name));

    int parent = getino(dirName);
    if(!parent)
    {
        printf("ERROR: Specified path does not exist\n");
        return 0;
    }
    MINODE * pip = iget(dev, parent);
    if(!S_ISDIR(pip->INODE.i_mode))
    {
        printf("ERROR: Specified path is not a directory\n");
        return 0;
    }
    if(search(pip, baseName))
    {
        printf("ERROR: Entry %s already exists\n", baseName);
        return 0;
    }

    if(dir)
    {
        pip->INODE.i_links_count++;
    }
    // creat (file) is similar to mkdir() except
    // (1) the INODE.i_mode field is set to regular type, permission bits set to 0644 = rw-r--r--
    // (2). no data block is allocated for it, s othe file size is 0
    // (3). links_count = 1; do not increment parent INODE's links_count
    
    int ino = ialloc(dev), bno = dir ? balloc(dev) : 0;

    MINODE * mip = iget(dev, ino);
    INODE * ip = &(mip->INODE);
    time_t timeHold = time(0L);

    *ip = (INODE){.i_mode = (dir ? 0x41ED : 0x81A4), 
    .i_uid = running->uid, 
    .i_gid = running->gid, 
    .i_size = (dir ? BLKSIZE : 0), 
    .i_links_count = (dir ? 2 : 1),
    .i_atime = timeHold, 
    .i_ctime = timeHold, 
    .i_mtime = timeHold, 
    .i_blocks = (dir ? 2 : 0), 
    .i_block = {bno}};
    
    mip->dirty = 1;

    if(dir)
    {
        char buf[BLKSIZE] = {0};
        char * cp = buf;
        DIR * dp = (DIR*)buf;
        *dp = (DIR){.inode = ino, 
        .rec_len = 12, .name_len = 1, 
        .name = "."};

        cp += dp->rec_len;
        dp = (DIR*)cp;
        *dp = (DIR){.inode = pip->ino, 
        .rec_len = BLKSIZE - 12, 
        .name_len = 2, 
        .name = ".."};

        put_block(dev, bno, buf);
    }

    enter_name(pip, ino, baseName);

    iput(mip);
    iput(pip);

    return ino;
}

// From pg. 336
void makedir(char *pathname)
{
    make(1, pathname);
}

// From pg. 336
void create_file(char *pathname)
{
    make(0, pathname);
}