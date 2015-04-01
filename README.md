Explain
=======

Explain is a command line tool that takes in a command with options and prints out what those options do, straight from the man page.

The motivation for this project is twofold:
1. I wanted to write something in C, for practice and fun
2. I was annoyed at so-called 'tutorials' that just list the commands they want you to run without telling you what they're really doing.

Using this tool, if you want to see what a command is going to do before running it, you need only pass it in.

Examples
--------
(todo)

How it works
------------
Explain actually invokes the man command as if you typed it yourself.  It caches the output to the /tmp directory, so subsequent calls to the same man page will not require re-opening the page (until /tmp is cleared).  Then it parses what it found line by line, searching for the arguments you included, printing them if it finds any.

But wait...
-----------
"...wouldn't Python have been a better choice?"

Yes, this program is very 'scriptable,' and I'm sure I could have done it in Python in a quarter of the lines and a third of the time.  The reason I wanted to do it in C is listed above under 'motivation.'

...also, for some reason the theme of this program fits C, for whatever that's worth.

Lastly, I'm not a pro with C by any means (which is likely clear from the source).  If you see something offensive I will welcome pull requests.
