/*  ANDREW GOEBEL
    -------------
    mychmod.c - A utility to apply a list of permission changes to a given
    list of files.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

enum { OTH, GRP, USR, NUM_SETS };

/* Structure for holding requested permission changes */
typedef struct permset
{
    mode_t adds;
    mode_t subs;
} permset;

/* Structure for holding list of files to be altered */
typedef struct filelist
{
    int list_capacity;
    int list_size;
    char ** filenames;
} filelist;

/* Function: add_to_file_list
 * Purpose: Routine to add a file to list of files for updating
 * Parameters: list - A pre-existing list structure for holding filenames
 *             filename - The new filename string to be added
 * Return value: 0 on success, 1 on error
*/

int add_to_file_list(filelist * list, const char * filename)
{
    /* Check list is large enough to hold another entry */
    if(list != NULL) 
    {
        if(list->list_size + 1 > list->list_capacity)
        {
            char ** new_filename_list;
    
            new_filename_list = realloc(list->filenames, sizeof(char *) * (list->list_capacity + 100));
            if(new_filename_list == NULL)
            {
                return 1;
            }
    
            list->filenames = new_filename_list;
            list->list_capacity = list->list_capacity + 100;
        }
    
        /* Add entry */
        list->filenames[list->list_size] = calloc(strlen(filename) + 1, sizeof(char));
        if(list->filenames[list->list_size] == NULL)
        {
            return 1;
        }
    
        strcpy(list->filenames[list->list_size], filename);
        list->list_size = list->list_size + 1;
    }

    return 0;
}

/* Function: free_file_list
 * Purpose: Release all memory holding list of files 
 * Parameter: filelist - Existing list of filenames
 * Return value: None
*/

void free_file_list(filelist * filelist)
{
    int freei;
    if(filelist != NULL)
    {
        if(filelist->filenames != NULL)
        {
            /* Free each individually allocated filename */
            for(freei = 0; freei < filelist->list_size; freei++)
            {
                free(filelist->filenames[freei]);
            }

            /* Free overall file list structure */
            free(filelist->filenames);
        }
    }
}

/* Function: add_change_to_set
 * Purpose: Add permission option to set of changes 
 * Parameters: set - Address of existing permission set
 *             arg - One command line string
 *             modeset - Indication of USR, GRP or OTH set being updated
 * Return value: 0 on success, 1 on error
*/

int add_change_to_set(mode_t * set, char * arg, int modeset)
{
    mode_t modechange = 0;
    int argi;

    /* Extract each permission character */
    for(argi = 0; argi < strlen(arg); argi++)
    {
        /* Add appropriate bit value */
        switch(arg[argi])
        {
           case 'r': modechange = modechange | 0x04; break;
           case 'w': modechange = modechange | 0x02; break;
           case 'x': modechange = modechange | 0x01; break;
           default:
               fprintf(stderr, "Error: Invalid permission option: %c\n", arg[argi]);
               return 1;
        }
    }

    /* Shift mode position depending on whether set is USER, GROUP or OTHER */
    switch(modeset)
    {
        case USR: modechange = modechange << 6; break;
        case GRP: modechange = modechange << 3; break;
    }

    /* Update existing set of changes */
    *set = *set | modechange;

    return 0;
}

/* Function: parse_cmd_line
 * Purpose: Read permission updates and file list from command line 
 * Parameters: argc - Number of arguments passed on the command line
 *             argv - Command line arguments
 *             perms - Permission set to be populated
 *             files - File list to be populated
 * Return value: 0 on success, 1 on parse error, 3 on flag error
*/

int parse_cmd_line(int argc, char ** argv, permset * perms, filelist * files)
{
    /* Check for minimum number of command line arguments */
    if(argc < 4)
    {
        fprintf(stderr, "Error: Insufficient number of arguments\n");
        return 1;
    }

    /* Read permission switches */
    int index;
    int c;
     
    while ((c = getopt (argc, argv, "u:g:o:U:G:O:")) != -1)
    {
        switch (c)
        {
            case 'u':
                if(add_change_to_set(&(perms->adds), optarg, USR) != 0) return 1;
                break;
            case 'g':
                if(add_change_to_set(&(perms->adds), optarg, GRP) != 0) return 1;
                break;
            case 'o':
                if(add_change_to_set(&(perms->adds), optarg, OTH) != 0) return 1;
                break;
            case 'U':
                if(add_change_to_set(&(perms->subs), optarg, USR) != 0) return 1;
                break;
            case 'G':
                if(add_change_to_set(&(perms->subs), optarg, GRP) != 0) return 1;
                break;
            case 'O':
                if(add_change_to_set(&(perms->subs), optarg, OTH) != 0) return 1;
                break;
            default:
                fprintf(stderr, "Error: Invalid Argument: %s\n", argv[0]);
                return 1;
        }
    }
    
    /* Treat all other arguments as filenames */ 
    for (index = optind; index < argc; index++)
    {
        if(add_to_file_list(files, argv[index]) != 0)
        {
            fprintf(stderr, "Error: Unable to add filename to list\n");
            return 1;
        }
    }

    /* Check permission arguments don't conflict */
    if( ((perms->adds & S_IRUSR) && (perms->subs & S_IRUSR)) ||
        ((perms->adds & S_IWUSR) && (perms->subs & S_IWUSR)) ||
        ((perms->adds & S_IXUSR) && (perms->subs & S_IXUSR)) )
    {
        fprintf(stderr, "Error: Conflicting USER permission options\n");
        return 3;
    }

    if( ((perms->adds & S_IRGRP) & (perms->subs & S_IRGRP)) ||
        ((perms->adds & S_IWGRP) & (perms->subs & S_IWGRP)) ||
        ((perms->adds & S_IXGRP) & (perms->subs & S_IXGRP)) )
    {
        fprintf(stderr, "Error: Conflicting GROUP permission options\n");
        return 3;
    }

    if( ((perms->adds & S_IROTH) & (perms->subs & S_IROTH)) ||
        ((perms->adds & S_IWOTH) & (perms->subs & S_IWOTH)) ||
        ((perms->adds & S_IXOTH) & (perms->subs & S_IXOTH)) )
    {
        fprintf(stderr, "Error: Conflicting OTHER permission options\n");
        return 3;
    }

    /* Check that files have been given */
    if(files->list_size == 0)
    {
        fprintf(stderr, "Error: No files given\n");
        return 1;
    }

    return 0;
}

/* Function: change_permissions
 * Purpose: Apply the given permissions to the given list of files
 * Parameters: files - Populated file list
 *             perms - Set of permission additions and removals
 * Return value: 0 on success, 2 on file access error, 4 on file exist error
*/

int change_permissions(filelist * files, permset * perms)
{
    int filei, ret = 0;
    mode_t newmode, removemodes;
    struct stat statbuf;

    for(filei = 0; filei < files->list_size; filei++)
    {
        /* Get current mode of file */
        if(stat(files->filenames[filei], &statbuf) != 0)
        {
            switch(errno)
            {
                case EACCES:
                    ret = 2;
                    fprintf(stderr, "Error: Access denied to %s\n",
                        files->filenames[filei]);
                    continue;
                case ENOENT:
                    ret = 4;
                    fprintf(stderr, "Error: %s does not exist\n",
                        files->filenames[filei]);
                    continue;
                default:
                    fprintf(stderr, "Error: Unexpected error while trying to access %s - %s\n",
                        files->filenames[filei], strerror(errno));
            }
        }

        /* Create new mode value */
        newmode = statbuf.st_mode;
        newmode = newmode | perms->adds;
        removemodes = newmode & perms->subs;
        newmode = newmode ^ removemodes;

        /* Apply new mode */
        if(chmod(files->filenames[filei], newmode) != 0)
        {
            switch(errno)
            {
                case EPERM:
                    ret = 2;
                    fprintf(stderr, "Error: Insufficient privileges to change %s\n",
                        files->filenames[filei]);
                    continue;
                case ENOENT:
                    ret = 4;
                    fprintf(stderr, "Error: %s does not exist\n",
                        files->filenames[filei]);
                    continue;
                default:
                    fprintf(stderr, "Error: Unexpected error while trying to change %s - %s\n",
                        files->filenames[filei], strerror(errno));
            }
        }
    }

    return ret;
}

/* Function: main
 * Purpose: Call all sub-routines
 * Parameters: Command line
 * Return value: 0 on success
 *               1 on parsing failure
 *               2 on file access error
 *               3 on flag conflict error
 *               4 on file existance error
*/

int main(int argc, char **argv)
{
    int returnval;
    permset perms;
    filelist files;
    memset(&perms, 0, sizeof(struct permset));
    memset(&files, 0, sizeof(struct filelist));

    /* Read and validate command line arguments */
    returnval = parse_cmd_line(argc, argv, &perms, &files);
    if(returnval != 0)
    {
        free_file_list(&files);
        fprintf(stderr, "Usage: mychmod [-u rwx] [-g rwx] [-o rwx] [-U rwx] [-G rwx] [-O rwx] <filename> [<filename>...]\n");
        return returnval;
    }

    /* Change file permissions */
    returnval = change_permissions(&files, &perms);

    /* Cleanup memory */
    free_file_list(&files);

    return returnval;
}
