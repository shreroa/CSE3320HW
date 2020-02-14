/*

Name: Bijay Raj Raut
ID:   1001562222

*/
// The MIT License (MIT)
//
// Copyright (c) 2016, 2017 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define  MAX_COMMAND_SIZE 255    // The maximum command-line size

#define  MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments
#define  BLOCK_SIZE 8192
#define  NUM_BLOCKS 4226
#define  NUM_FILES  128
#define  MAX_FILE_SIZE 10240000
#define MAX_FILENAME_LENGTH 32
//File descriptor to open and close the file
FILE *fd;
//All the data for the mav file system will be stored in blocks
//including inodes
uint8_t blocks[NUM_BLOCKS][BLOCK_SIZE];

//structure to hold the directry entries with flags to identify it
struct Directory_Entry
{
    int8_t valid;
    char filename [MAX_FILENAME_LENGTH];
    uint32_t inode;
    time_t time;
};

//structure to hold the inode entries
struct Inode
{
    uint8_t valid;
    uint8_t attributes; //0 default; 1 hidden; 2 read only
    uint32_t size;
    uint32_t blocks[1250];
};
//struct Directory_Entry dir[NUM_FILES];
struct Directory_Entry * dir; //dir points at block 0
struct Inode           * inodes; //inodes list
uint8_t                * freeBlockList; //free block list
uint8_t                * freeInodeList; //free inode list

//memsets all the name field and sets the inode to -1
void initializeDirectory();
void initializeInodes();

//initializing free Block and inode list and setting
void initializeBlockList();
void initializeInodeList();

//query functions to get the index of free directory node and block
int findFreeDirectory(char *filename);
int findFreeInode();
int findFreeBlock();

//helper method
//returns the directory index from the directory for the given filename
int getdirindex(char *filename);

//create the file with given name

//open the file system image and close the file system image
//open shall open the file system and copy the data to the
//2d array of blocks and close the file.
void open( char *filename);
//close shall open the file system and write all the changes
//that were made to the file system and close the file
void fileclose(char * filename);
//creates a brand new file system image of any specied name
//CAUTION: It shall overwrite the disk image file if their exist
//one with the similar name scheme
void createfs  (char * filename);

//put command will put the desired file into the file system
//basically it will be copying the data to the blocks array which
//can be later written to the file system
//put shall check the file's size and will not accept the input if the
//file size is big enough to fit in the remaining space or the file is
//bigger than MAX_FILE_SIZE or the filename is longer than MAX_FILENAME_LENGTH
void put(char *filename);
//get will retrieve the file and copy it to the current directory
//if the file name is provided
//if the newfile name is absent; get will use the provided filename
//and if exists overwrite the existing file or create the new file
//with following name
void get(char *filename,char *newfilename);
//counts the amount of free space that the disk currently have
//by iterating over the freeBlockLIst and checking the flags
//returns the free available disk size in bytes
int df();
//lists all the files in the filesystem
//except the hidden files
//void list();
//changes the attributes of the files
//0 is default
//1 hidden
//2 read only
void attrib(char *filename, uint8_t newatrib);
void del(char *filename);
void list(int hide);

void startinitializer();


int main()
{
    char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

    startinitializer();
    //printf("%hhd,%hhd,%hhu,free: %d\n",dir[0].valid,dir[1].valid,freeInodeList[0],df());
    //freeBlockList[2]=0;
    //printf("%hhd,%hhd,%hhu,free: %d\n",dir[0].valid,dir[1].valid,freeInodeList[0],df());
    while( 1 )
    {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    if(cmd_str[0]=='\n'||cmd_str[0]==' ')
        continue;

    //Parse input
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;

    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;

    char *working_str  = strdup( cmd_str );

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) &&
              (token_count<MAX_NUM_ARGUMENTS))
    {
        token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
        if( strlen( token[token_count] ) == 0 )
        {
          token[token_count] = NULL;
        }
        token_count++;
    }
    if(strcmp(token[0],"quit")==0)
    {
        break;
    }
    else if(strcmp(token[0],"put")==0)
    {
        if(token_count<3)
        {
            printf("error: Invalid format.\nput <filename>\n");
            continue;
        }
        put(token[1]);
    }
    else if(strcmp(token[0],"get")==0)
    {
        if(token_count<3||token_count>4)
        {
            printf("error: Invalid format.\nget <filename>\nget <filename> <newfilename>\n");
            continue;
        }
        if(token_count==4)
            get(token[1],token[2]);
        else
            get(token[1],"NOFILENAME");
    }
    else if(strcmp(token[0],"del")==0)
    {
        if(token_count<3)
        {
            printf("error: No file name provided.\n");
            continue;
        }
        del(token[1]);
    }
    else if(strcmp(token[0],"list")==0)
    {
        if(token_count==3)
        {
            list(1);
        }
        else
        {
            list(0);
        }
    }
    else if(strcmp(token[0],"df")==0)
    {
        printf("%d bytes free.\n",df());
    }
    else if(strcmp(token[0],"open")==0)
    {
        if(token_count<3)
        {
            printf("error: No file name provided.\n");
            continue;
        }
        open(token[1]);
    }
    else if(strcmp(token[0],"close")==0)
    {
        if(token_count<3)
        {
            printf("error: No file name provided.\n");
            continue;
        }
        fileclose(token[1]);
    }
    else if(strcmp(token[0],"createfs")==0)
    {
        if(token_count<3)
        {
            printf("error: No file name provided.\n");
            continue;
        }
        createfs(token[1]);
    }
    else if(strcmp(token[0],"attrib")==0)
    {
        if(token_count<4)
        {
            printf("error: Invalid format.\nattrib [+attribute][-attribute] <filename>\n");
            continue;
        }
        if(strcmp(token[1],"+r")==0)
        {
            attrib(token[2],2);
        }
        else if(strcmp(token[1],"-r")==0)
        {
            attrib(token[2],0);
        }
        else if(strcmp(token[1],"+h")==0)
        {
            attrib(token[2],1);
        }
        else if(strcmp(token[1],"-h")==0)
        {
            attrib(token[2],0);
        }
        else
        {
            printf("attrib: unsupported request.\nfs supports +h|-h|+r|-r\n");
        }

    }
    free( working_root );

    }
    return 0;
}

void initializeDirectory()
{
    int i;
    for(i = 0; i < NUM_FILES; i++)
    {
       // dir[i] = (struct Directory_Entry * )malloc(sizeof(struct Directory_Entry));
        dir[i].valid = 0;
        //printf("%d. valid = %d\n",i,dir[i].valid);
        memset( dir[i].filename,0, 32);
        dir[i].inode = -1;
    }
}

void initializeInodes()
{
    int i;
    for(i = 3; i< 131; i++)
    {
        //inodes = (struct Inode*)blocks[i];
        int j;
        inodes[i].valid      = 0;
        inodes[i].attributes = 0;
        inodes[i].size       = 0;
        for (j = 0; j<1250; j++)
        {
            inodes[i].blocks[j] = -1;

        }
    }
}

void initializeBlockList()
{
    int i;
    for(i = 0; i < NUM_BLOCKS; i++)
    {
        freeBlockList[i] = 1;
    }
}
void initializeInodeList()
{
    int i;
    for(i = 0; i < NUM_FILES; i++)
    {
        freeInodeList[i] = 1;
    }
}

int findFreeDirectory(char *filename)
{
    int ret = -1;
    int i;

    for (i = 0; i < NUM_FILES; i++)
    {
        if(strcmp(dir[i].filename,filename) == 0)
        {
            ret = i;
            break;
        }
    }
    if(ret==-1)
    {
        for (i = 0; i < NUM_FILES; i++)
        {
            if(dir[i].valid == 0)
            {
                ret = i;
                dir[i].valid = 1;
                break;
            }
        }
    }
    return ret;
}

int findFreeInode()
{
    int ret = -1;
    int i;

    for(i = 0; i < NUM_FILES; i++)
    {
        if(inodes[i].valid == 0)
        {
            inodes[i].valid = 1;
            freeInodeList[i] = 0;
            ret = i;
            break;
        }
    }

    return ret;
}

int findFreeBlock()
{
    int ret = -1;
    int i;

    for (i = 10; i<NUM_BLOCKS; i++)
    {
        if(freeBlockList[i] == 1)
        {
            freeBlockList[i] = 0;
            return i;
            //break;
        }
    }
    return ret;
}
//helper function
int getdirindex(char *filename)
{
    int i;
    //searches the directory for the given filename and
    //returns the index of the file's saves directory
    for( i = 0; i < NUM_FILES; i++)
    {
        if(strcmp(dir[i].filename,filename)==0)
        {
            return i;
        }
    }
    return -1;
}

int df()
{
    int i;
    int free = 0;
    for(i = 0; i < NUM_BLOCKS; i++)
    {
        if(freeBlockList[i]==1)
            free+=BLOCK_SIZE;
    }
    return free;
}

void get(char *filename,char *newfilename)
{
    //printf("here %s ,%s\n",filename,newfilename);
    //returns the directory index
    //if not found returns -1;
    int check = getdirindex(filename);
    //printf("Dir: %d\n",check);
    //printf("Filename: %s\n",filename);
    if( check == -1)
    {
        printf("get error: File not found.\n");
        return;
    }
    if(strcmp(newfilename,"NOFILENAME")==0)
    {
        newfilename = (char *)malloc(strlen(filename));
        strcpy(newfilename,filename);
    }
    //printf("here\n");
    FILE *ofp;
    ofp = fopen(newfilename,"w");
    if( ofp == NULL )
    {
        perror("Opening output file returned");
        return;
    }
    int j=0;
    // Initialize our offsets and pointers just we did above when reading from the file.

    int block_index = inodes[dir[check].inode].blocks[j++];
    int copy_size   = inodes[dir[check].inode].size;
    int offset      = 0;

    //printf("Writing %d bytes to %s\n", (int) buf . st_size, argv[2] );

    // Using copy_size as a count to determine when we've copied enough bytes to the output file.
    // Each time through the loop, except the last time, we will copy BLOCK_SIZE number of bytes from
    // our stored data to the file fp, then we will increment the offset into the file we are writing to.
    // On the last iteration of the loop, instead of copying BLOCK_SIZE number of bytes we just copy
    // how ever much is remaining ( copy_size % BLOCK_SIZE ).  If we just copied BLOCK_SIZE on the
    // last iteration we'd end up with gibberish at the end of our file.
    while( copy_size > 0 )
    {

      int num_bytes;

      // If the remaining number of bytes we need to copy is less than BLOCK_SIZE then
      // only copy the amount that remains. If we copied BLOCK_SIZE number of bytes we'd
      // end up with garbage at the end of the file.
      if( copy_size < BLOCK_SIZE )
      {
        num_bytes = copy_size;
      }
      else
      {
        num_bytes = BLOCK_SIZE;
      }

      // Write num_bytes number of bytes from our data array into our output file.
      fwrite( blocks[block_index], num_bytes, 1, ofp );

      // Reduce the amount of bytes remaining to copy, increase the offset into the file
      // and increment the block_index to move us to the next data block.
      copy_size -= BLOCK_SIZE;
      offset    += BLOCK_SIZE;
      //only increment the block array if we might have further data in the array
      //which we may know from the copy size
      if(copy_size>0)
        block_index = inodes[dir[check].inode].blocks[j++];

      // Since we've copied from the point pointed to by our current file pointer, increment
      // offset number of bytes so we will be ready to copy to the next area of our output file.
      fseek( ofp, offset, SEEK_SET );
    }

    // Close the output file, we're done.
    fclose( ofp );
}

void put(char *filename)
{
    //checks the length of the filename provided by the user
    //if the file's name's length is greater than MAX_FILENAME_LENGTH
    //the put will generate an error message to the user and will not execute
    if(strlen(filename)>MAX_FILENAME_LENGTH)
    {
        printf("put error: File name too long.\n");
        return;
    }
    struct stat buf;
    int ret;
    ret = stat(filename, &buf);
    //if file does not exist ret will have -1
    //else we will get a good value
    //and the buffer will be populated
    //off_t st_size will return the size of the file

    //first checking if the file exist
    //put will generate an error if the file with the given name does not exist
    if(ret == -1)
    {
        printf("put error: %s doesnot exist.\n",filename);
        return;
    }
    //if file does exist check if it exceeds teh max file size that our fs supports
    else if(buf.st_size > MAX_FILE_SIZE)
    {
        printf("put error: File %s is too big.\n",filename);
        return;
    }
    //check if our fs has enough space to save the file
    else if(buf.st_size > df())
    {
        printf("put error: Not enough disk space.\n");
        return;
    }
    //put the file in the image
    //block copy file
    else
    {
        //find the index of the free directory in the blocks to store the file
        int directoryindex = findFreeDirectory(filename);
        //if no free directory is found generate an error
        if(directoryindex == -1)
        {
            printf("put error: No Free Directory.\n");
            return;
        }
        //find free inode to store the file blocks indexes
        int inodeIndex = findFreeInode();
        //if theres no free inode left then generate an error
        if(inodeIndex == -1)
        {
            printf("put error: No Free Inodes found");
            return;
        }

        FILE *ifp = fopen( filename, "r" );
        //creating time_t type variable to retrieve the current time
        time_t accesstime;
        accesstime =time(NULL); //retrieving the current time
        //save the time to the directory entry

        dir[directoryindex].time=accesstime;
        //saving the directory and inodes information
        dir[directoryindex].inode=inodeIndex;
        strcpy(dir[directoryindex].filename,filename);
        //add time as well
        inodes[inodeIndex].attributes=0; //0 as not hidden
        inodes[inodeIndex].size = buf.st_size;

        //printf("Data %s size %d : dirindex: %d, inode: %d\n",dir[directoryindex].filename,buf.st_size,directoryindex,inodeIndex);

        // Save off the size of the input file since we'll use it in a couple of places and
        // also initialize our index variables to zero.
        int copy_size   = buf . st_size;

        // We want to copy and write in chunks of BLOCK_SIZE. So to do this
        // we are going to use fseek to move along our file stream in chunks of BLOCK_SIZE.
        // We will copy bytes, increment our file pointer by BLOCK_SIZE and repeat.
        int offset      = 0;

        // We are going to copy and store our file in BLOCK_SIZE chunks instead of one big
        // memory pool. Why? We are simulating the way the file system stores file data in
        // blocks of space on the disk. block_index will keep us pointing to the area of
        // the area that we will read from or write to.
        int block_index  = 0;
        int blockindex   = 0; //increment the blocks array starting from 0

        // copy_size is initialized to the size of the input file so each loop iteration we
        // will copy BLOCK_SIZE bytes from the file then reduce our copy_size counter by
        // BLOCK_SIZE number of bytes. When copy_size is less than or equal to zero we know
        // we have copied all the data from the input file.
        while( copy_size > 0 )
        {
            if(copy_size > 0)
            {
                block_index = findFreeBlock();
                inodes[inodeIndex].blocks[blockindex++]=block_index;
            }
            // Index into the input file by offset number of bytes.  Initially offset is set to
            // zero so we copy BLOCK_SIZE number of bytes from the front of the file.  We
            // then increase the offset by BLOCK_SIZE and continue the process.  This will
            // make us copy from offsets 0, BLOCK_SIZE, 2*BLOCK_SIZE, 3*BLOCK_SIZE, etc.
            fseek( ifp, offset, SEEK_SET );

            // Read BLOCK_SIZE number of bytes from the input file and store them in our
            // data array.
            int bytes  = fread(blocks[block_index], BLOCK_SIZE, 1, ifp );

            // If bytes == 0 and we haven't reached the end of the file then something is
            // wrong. If 0 is returned and we also have the EOF flag set then that is OK.
            // It means we've reached the end of our input file.
            if( bytes == 0 && !feof( ifp ) )
            {
                printf("An error occured reading from the input file.\n");
                return;
            }

            // Clear the EOF file flag.
            clearerr( ifp );

            // Reduce copy_size by the BLOCK_SIZE bytes.
            copy_size -= BLOCK_SIZE;

            // Increase the offset into our input file by BLOCK_SIZE.  This will allow
            // the fseek at the top of the loop to position us to the correct spot.
            offset += BLOCK_SIZE;

            // Increment the index into the block array
            //block_index = findFreeBlock();
        }

        // We are done copying from the input file so close it out.
        fclose( ifp );
        //printf("\nDir status: %s of %d on dirindex %d and inode index %d",dir[directoryindex].filename,inodes[dir[directoryindex].inode].size,directoryindex,dir[directoryindex].inode);
        //printf("\nNode status: valid %d,attribute %d,size %d\n",inodes[dir[directoryindex].inode].valid,inodes[dir[directoryindex].inode].attributes,inodes[dir[directoryindex].inode].size);
    }
}


void attrib(char *filename, uint8_t newatrib)
{
    //using getdirindex() to retrieve the directory index of the provided filename
    //if getdirindex() returns -1, return the function as the file does not exist

    int index = getdirindex(filename);
    if(index == -1)
    {
        printf("attrib error: File not found.\n");
        return;
    }
    inodes[dir[index].inode].attributes = newatrib;
}
void del(char *filename)
{
    int index;
    //use the helper method to find the index of the user provided
    //file name and if the file does not exist generate an error
    index = getdirindex(filename);
    if(index == -1)
    {
        printf("del error: File not found.\n");
        return;
    }
    //check if the file is marked read only or not
    //if the file is indeed read only alert user
    //about the situation and do not delete the file
    if(inodes[dir[index].inode].attributes == 2)
    {
        printf("del: That file is marked read-only.\n");
        return;
    }
    //to delete we are simply going to set the validity of the
    //block to be 1:marking it as free to overwrite it and
    //directory index and inode index to be 0 and memset the filename
    //without the indexes the file will not appear in the file system
    //and cannot be retrieved, Thus: gets deleted
    int i;
    int copy_size = inodes[dir[index].inode].size;
    while(copy_size > 0)
    {
        uint32_t blockindex = inodes[dir[index].inode].blocks[i++];
        freeBlockList[blockindex] = 1;
        copy_size -= BLOCK_SIZE;
        //printf("i = %d blockindex = %d\n",i,blockindex);
    }
    inodes[dir[index].inode].valid         = 0;
    freeInodeList[dir[index].inode]        = 1;
    dir[index].valid                       = 0;
    memset(dir[index].filename,0,32);
    //printf("%d\n",counter);
}
void startinitializer()
{
    dir            = (struct Directory_Entry*)&blocks[0]; //Directory on block 0
    freeInodeList  = (uint8_t*)&blocks[7]; //free inode map
    freeBlockList  = (uint8_t*)&blocks[8]; //free Block map
    inodes         = (struct Inode*)&blocks[9]; //inodes starting at block 3
    /*int i;
    for(i = 3; i< 131;i++,inodes++)
    {
        inodes = (struct Inode*)&blocks[i];
    }*/
    initializeDirectory();
    initializeInodes();
    initializeInodeList();
    initializeBlockList();

    //marking blocks
    freeBlockList[7]=0;
    freeBlockList[8]=0;
    freeBlockList[9]=0;

    //marking block
    //dir[0].valid = 1;
    //dir[1].valid = 1;
}

void open( char *filename)
{
    fd  = fopen (filename, "r");
    if(fd == NULL)
    {
        printf("open: File not found\n");
        return;
    }
    fread(&blocks, BLOCK_SIZE, NUM_BLOCKS, fd);
    fclose( fd );

}
void fileclose(char * filename)
{
    fd = fopen(filename,"w");
    if(fd == NULL)
    {
        printf("close: File not found\n");
        return;
    }
    fwrite(&blocks, BLOCK_SIZE, NUM_BLOCKS, fd);
    fclose( fd );
}
void createfs  (char * filename)
{
    int i;

    memset(&blocks[0], 0, NUM_BLOCKS * BLOCK_SIZE);
    //memset(&blocks[0], 0, NUM_BLOCKS * BLOCK_SIZE);

    fd = fopen (filename , "w" );
    if(fd==NULL)
        exit(-1);
    startinitializer();
    fwrite(&blocks[0], BLOCK_SIZE, NUM_BLOCKS, fd);
    //fwrite(&blocks[0], BLOCK_SIZE, NUM_BLOCKS, fd);

    fclose( fd );
}
void list(int hide)
{
    int i;
    int count=0;
    char time[15];
    struct tm *gettime; //struct tm pointer to get the saved time

    for(i = 0; i < NUM_FILES; i++)
    {
        //printf("%d. Valid = %d\n",i,dir[i].valid);
        //if valid and not hidden
        if(dir[i].valid==1)
        {
            int inodeIndex = dir[i].inode;
            //printf("Index: %d",inodeIndex);
            if(hide==0)
            {
                //localtime will return the saved time from the inode
                //and using stftime we can retrieve the month date hour and min
                //factors of the file's saving time: Month Date Hour:Minute
                if(inodes[inodeIndex].attributes!=1)
                {
                    gettime = localtime(&dir[i].time);
                    strftime(time,sizeof(time),"%b %d %H:%M",gettime);
                    printf("%-5u %s %s\n",inodes[dir[i].inode].size,time,dir[i].filename);
                    count++;
                }
            }
            else
            {
                gettime = localtime(&dir[i].time);
                strftime(time,sizeof(time),"%b %d %H:%M",gettime);
                printf("%-5u %s %s\n",inodes[dir[i].inode].size,time,dir[i].filename);
                count++;
            }
        }
    }
    if(count==0)
    {
        printf("list: No files found.\n");
    }
    //printf("valid=0: %d\nvalid=1 : %d\n",count,c);
}
