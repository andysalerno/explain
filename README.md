Explain
=======

Explain is a command line tool that takes in a command with options and prints out what those options do, straight from the man page.

The motivation for this project is twofold:

1. I wanted to write something in C, for practice and fun

2. I was annoyed at so-called 'tutorials' that just list the commands they want you to run without telling you what they're really doing.

Using this tool, if you want to see what a command is going to do before running it, you need only pass it in.

Examples
--------
    $ explain ls -alh --author
    ls - list directory contents
    -a, --all
           do not ignore entries starting with .

    --author
           with -l, print the author of each file

    -h, --human-readable
           with -l, print sizes in human readable format (e.g., 1K 234M 2G)

    -l     use a long listing format

How it works
------------
Explain actually invokes the man command as if you typed it yourself.  It caches the output to the /tmp directory, so subsequent calls to the same man page will not require re-opening the page (until /tmp is cleared).  Then it parses what it found line by line, searching for the arguments you included, printing them if it finds any.

But wait...
-----------
"...wouldn't Python have been a better choice?"

Yes, this program is very 'scriptable,' and I'm sure I could have done it in Python in a quarter of the lines and a third of the time.  The reason I wanted to do it in C is listed above under 'motivation.'

...also, for some reason the theme of this program fits C, for whatever that's worth.
