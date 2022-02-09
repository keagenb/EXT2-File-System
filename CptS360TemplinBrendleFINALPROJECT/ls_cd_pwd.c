// list directory, change directory, get CWD path (as outline in pg. 3 of "FP_CptS360_Spring2021.docx")

/************* cd_ls_pwd.c file **************/

#include "ls_cd_pwd.h"

// From pg. 330
void mychdir(char *pathname)
{
  // printf("under construction READ textbook HOW TO chdir!!!!\n");
  // READ Chapter 11.7.3 HOW TO chdir
  
    if (strlen(pathname) == 0 || strcmp(pathname, "/") == 0)
    {
        iput(running->cwd);
        running->cwd = root;
        return;
    }
    else
    {
        char temp[256];
        strcpy(temp,pathname);
        if(pathname[0] == '/')
        {
            dev = root->dev;
        }
        else
        {
            dev = running->cwd->dev;
        }
        // (1). int ino = getino(pathname); // return error if ino = 0
        int ino = getino(temp);
        // (2). MINODE *mip = iget(dev, ino);
        MINODE *mip = iget(dev, ino);
        // (3). Verify mip->INODE is a DIR // return error if not DIR
        if(!(S_ISDIR(mip->INODE.i_mode)))
        {
            printf("%s is not a directory", pathname);
            return;
        }
        // (4). iput(running->cwd); // release old cwd
        iput(running->cwd);
        // (5). running->cwd = mip // change cwd to mip
        running->cwd = mip;
    }
}

// From pg. 329
void ls_file(int ino, char *filename)
{
    // printf("ls_file: to be done: READ textbook for HOW TO!!!!\n");
    // READ Chapter 11.7.3 HOW TO ls
    // ls_file(filename) list the STAT information of the file
    MINODE * mip = iget(dev, ino);
    const char * t1 = "xwrxwrxwr-------";
    const char * t2 = "----------------";
    if(S_ISREG(mip->INODE.i_mode))
        putchar('-');
    else if(S_ISDIR(mip->INODE.i_mode))
    {
        putchar('d');
    }
    else if(S_ISLNK(mip->INODE.i_mode))
    {
        putchar('l');
    }
    for(int i = 8; i >= 0; i--)
    {
        putchar(((mip->INODE.i_mode) & (1 << i)) ? t1[i] : t2[i]);
    }
    time_t t = (time_t)(mip->INODE.i_ctime);
    char temp[256];
    strcpy(temp, ctime(&t))[strlen(temp) - 1] = '\0';

    printf(" %4d %4d %4d %4d %s %s", (int)(mip->INODE.i_links_count), (int)(mip->INODE.i_gid), (int)(mip->INODE.i_uid), (int)(mip->INODE.i_size), temp, filename);

    if(S_ISLNK(mip->INODE.i_mode))
    {
        printf(" -> %s", (char*)(mip->INODE.i_block));
    }

    putchar('\n');

    iput(mip);   
}

// From pg. 329
void ls_dir(char *dirname)
{
    // printf("ls_dir: list CWD's file names; YOU do it for ls -l\n");
    int ino = getino(dirname);
    MINODE * mip = iget(dev, ino);
    // ls_dir(dirname) use opendir() and readdir(), call ls_file(filename) for each file
    char buf[BLKSIZE];
    DIR *dp;
    char *cp;
    for(int i = 0; i < 12 && mip->INODE.i_block[i]; i++)
    {
        // Assume DIR has only one data block i_block[0]
        get_block(dev, mip->INODE.i_block[i], buf);
        dp = (DIR*)buf;
        cp = buf;

        while(cp < buf + BLKSIZE)
        {
            char temp[256];
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
            printf("[%d %s]  ", dp->inode, temp); // print [inode# name]
            ls_file(dp->inode, temp);
            
            cp += dp->rec_len;
            dp = (DIR*)cp;
        }
    }
    iput(mip);
    printf("\n");
}

// From pg. 330
void ls(char *pathname)
{
    //printf("ls %s\n", pathname);
    //printf("ls CWD only! YOU do it for ANY pathname\n");
    char temp[256];
    int ino;
    if(strlen(pathname) == 0)
    {
        strcpy(pathname, ".");
        ino = running->cwd->ino;
    }
    else
    {
        strcpy(temp, pathname);
        ino = getino(temp);
    }
    // For each dir_entry use iget() to get its minode
    MINODE *mip = iget(dev, ino);
    // Then call ls_file (or ls_dir)
    if(S_ISDIR(mip->INODE.i_mode))
    {
        ls_dir(pathname);
    }
    else
    {
        ls_file(ino, pathname);
    }
    iput(mip);
}

// From pg. 331
void rpwd(MINODE *wd)
{
    // rpwd(MINODE *wd) {}
    char my_name[256];
    // (1). if(wd == root) return;
    if (wd==root)
    {
        return;
    }
    // (2). from wd->INODE.i_block[0], get my_ino and parent_ino
    int parent_ino = search(wd,"..");
    int my_ino = wd->ino;
    // (3). pip = iget(dev, parent_ino);
    MINODE * pip = iget(dev, parent_ino);
    // (4). from pip->INODE.i_block[]: get my_name string by my_ino as LOCAL
    findmyname(pip, my_ino, my_name);
    // (5). rpwd(pip); // recursive call rpwd(pip) with parent minode
    rpwd(pip);
    iput(pip);
    // (6). print"/%s", my_name;
    printf("/%s", my_name);
}

// From pg. 331
void pwd()
{
  // printf("pwd: READ HOW TO pwd in textbook!!!!\n");
  // pwd(MINODE *wd) (if (wd == root) print "/"; else rpwd(wd) })
  // pwd start: pwd(running->cwd);
    if(running->cwd == root)
    {
        printf("/\n");
        return;
    }
    rpwd(running->cwd);
    printf('\n');
}