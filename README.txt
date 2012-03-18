Welcome to LyonPotpourri.

This installation uses a Perl script called "compileme.pl" to compile the externals and put them someplace useful. Open "compileme.pl" in an editor as you will need to make two modifications to this file. First, select as your platform either "linux" or "darwin". Second set $PD_ROOT to the path of your Pd directory. On linux this will often be "/usr/lib/pd". On darwin it will usually be something like 
"/Applications/Pd-0.40-2.app".

Save and close "compileme.pl".

In order to compile all the externals, type:

$ perl compileme.pl make

If everything goes well, then type:

$ perl compileme.pl install

This last command will copy all the externals to Pd. It will also copy all of the help files to Pd and will install some drum samples to the Pd sound directory that are needed by a few of the helpfiles. If you do not have write permission in some or all of the Pd directories, you will need to run the install command as root.

You can install the entire help folder "8.LyonPotpourri" to the docs directory for more convenient help browsing, with the following command:

$ perl compileme.pl install_helpfolder

You can clean everything up with:

$ perl compileme.pl clean

You can run any of these options with no action taken with -n, for example

$ perl compileme.pl -n make

Eric Lyon
e.lyon@qub.ac.uk
Department of Music and Sonic Arts
Queen's University Belfast
17th March 2007
