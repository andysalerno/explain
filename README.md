Explain
=======

Explain is a command line tool that takes in a command with options and prints out what those options do, straight from the man page.

The motivation for this project is twofold:

1. I wanted to write something in C, for practice and fun

2. I was annoyed at so-called 'tutorials' that just list the commands they want you to run without telling you what they're really doing.

Using this tool, if you want to see what a command is going to do before running it, you need only pass it in.

Installing
----------
Just compile it with make and put the output somewhere your PATH variable knows about.

Examples
--------
    [andy@localhost explain]$ explain ls -alh --author
       ls - list directory contents
       -a, --all
              do not ignore entries starting with .

       --author
              with -l, print the author of each file

       -h, --human-readable
              with -l, print sizes in human readable format (e.g., 1K 234M 2G)

       -l     use a long listing format


    [andy@localhost explain]$ explain tar -zxv
       tar - an archiver tool
       -x, --extract, --get
              extract files from an archive

       -z, --gzip, --gunzip, --ungzip
              filter the archive through gzip

       -v, --verbose
              verbosely list files processed

How it works
------------
Explain actually invokes the man command as if you typed it yourself.  It caches the output to the /tmp directory, so subsequent calls to the same man page will not require re-opening the page (until /tmp is cleared).  Then it parses what it found line by line, searching for the arguments you included, printing them if it finds any.

Known Bugs
----------
Explain doesn't have an intelligent engine behind the lines it prints--it considers any line that begins with a dash to be a potential option description.  For example, the following is undesired output:

    [andy@localhost explain]$ explain tar -f
           tar - an archiver tool
           --format=gnu  -f-  -b20  --quoting-style=escape  --rmt-command=/sbin/rmt   --rsh-com‐
       mand=/usr/bin/rsh

           -f, --file=ARCHIVE
                  use archive file or device ARCHIVE

           -f, --file=ARCHIVE
                  use archive file or device ARCHIVE

The long ugly line is printed because it starts with a dash, and contains '-f,' so it appears like it should be a match when it really isn't.  The duplicate -f description is printed simply because it appears in the man page twice. The second problem can be solved easily, the first one might just be the reality of having no absolute standard for man page formatting.

But wait...
-----------
"...wouldn't Python have been a better choice?"

Yes, this program is very 'scriptable,' and I'm sure I could have done it in Python in a quarter of the lines and a third of the time.  The reason I wanted to do it in C is listed above under 'motivation.'

...also, for some reason the theme of this program fits C, for whatever that's worth.
