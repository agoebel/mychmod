INTRODUCTION
============
mychmod is a tool for applying file permission changes to a list of files.

mychmod will attempt to apply the permission changes to all files given on the
command line. Errors will be reported for any failures, but all changes will
be attempted before the utility exits.

INSTALLATION
============

Compile the executable with the following command
gcc -o mychmod mychmod.c 

USE
===

The complete set of allowable arguments is as follows:

mychmod [-u rwx] [-g rwx] [-o rwx] [-U rwx] [-G rwx] [-O rwx] <filename(s)> 

-u/-U Add/Remove read, write and execute permissions for the owner of the file

-g/-G Add/Remove read, write and execute permissions for the group of the file

-o/-O Add/Remove read, write and execute permissions for all other users

Examples:
---------
- mychmod -O rwx doc - Prevent access to users outside of my group
                 
- mychmod -u x -g x o x app - Add execute permissions for all users
        
- mychmod -u rw -G rwx -O rwx passwd - Allow access only to the owner of the file

ERROR MESSAGES
==============

These are the more important error messages that can be encountered during use
of mychmod:

Conflicting USER permission options - These errors can occur when the user
Conflicting GROUP permission options - requests two changes that cannot both
Conflicting OTHER permission options - be applied. For example, -u r -U r

<filename> does not exist - Changes have been requested to a file that could not be found

Access denied to <filename> - The user is trying to change a file for which they do not have read access

Insufficient privileges to change <filename> - The user is trying to change a file for which they do not have write access

EXIT CODES
==========

0 = Complete Success
1 = The command line could not be successfully parsed or no files were specified
2 = File access error 
3 = Conflicting permission options
4 = Specified file(s) do not exist

INCLUDES
========

stdio.h - Printing to the console with fprintf
stdlib.h - Memory management functions : calloc, realloc, free
string.h - String functions: strlen, strcpy, strerror, memset
unistd.h - Command line parsing with getopt
sys/stat.h - File management routines: stat, chmod
errno.h - Error reporting with the errno global variable