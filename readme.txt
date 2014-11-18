NDB ("no double blocks")
========================


Table Of Contents

0. Introduction - why don't we do our backups?
1. What's the package NDB?
2. OS/2, Windows and Linux
3. Examples/Quick Guide
4. Tips
5. Usage (Complete Guide)
6. Limits
7. Technical Details
8. Known Bugs
9. Planned Further Development


0. Introduction - why don't we do backups?
==========================================

Everybody knows how important backups are - viruses, disk crashes, wrong user
commands - there are numerous reasons for up-to-date backups.

And in many cases it is very useful, not only to have a backup from the last
day, but also from earlier times:

- where is my last working version of program ABC ...
- last month I deleted file ABC, but I would need it now ...
- what did I change from ABC to DEF ...

All these points are reasons to keep the older backups also, but if you do so,
you need a lot of disk space, burned CDs, backup tapes or similar.

Or you have to change your strategy and have to switch between full backups
and differential or incremental backups. Differential or incremental backups
need less time and backup size, but if you have to restore data, the handling
is more complicated than with full backups.

The best would be a mixture of the advantages of full backups (simple
handling, simple data restoration) with the differential or incremental
backups (less time, less space) and the avoidance of the disadvantages of full
backups (much time, much space) and the differential or incremental backups
(more complicated handling, combination of different backups for restoring
data).

NDB is trying to get a mixture of the best qualities - only the first backups
needs time and space, all following backups are quick and small and all
backups are full backups.



1. What's the package NDB?
==========================


The four NDB executables are a package for the disk space friendly creation of
several full backups of the same or similar files. The archive files can be
created, viewed and extracted with OS/2, Win32 or Linux.

While archiving or extraction NDB takes care about the EAs of OS/2, the NTFS
security of WinNT (and above) and symbolic links, UID and GID of Linux.

NDB is able to deal with files of more then 2 GB size, this is true for OS/2
also!

An example is often more helpful than many words - this is the content of my
archive file back_ecs.ndb, where I save my main operation system OS/2:

Chapter List:
 No              Date  Files       Size     Packed Ratio                Name
---  ----------------  -----  ---------  --------- -----  ------------------
  0  2003-10-23 21:51   8468  412826898  209809856   49%  LW q: FP3
                                            Arbeits-eCS FP3 auf Hauptrechner
  1  2003-10-23 21:59   7029  320727813   34581644   89%  Wartung eCS j:
           Wartungspartition auf Hauptrechner, frisch installiert, Fixpack 1
  2  2003-10-28 21:22   8872  376653288   34043040   90%  SrvOld Q: eCS
                                      eCS FP3 LW Q: auf Rechner "Server alt"
  3  2003-10-28 22:31   7184  323455049    7490184   97%  Wartung eCS j:
            Wartungspartition auf Hauptrechner nach Einspielen von Fixpack 3
  4  2003-12-07 22:41   8642  413709050    1075664   99%  LW q: FP3
                            nach Einspielen diverses Mozilla 1.4.1-Versionen
  5  2004-01-31 10:50   8766  466151424     964516   99%  LW q: FP3
                                                 Netz stÅrzt beim Drucken ab
  6  2004-01-31 15:39   8771  449503113    1459808   99%  LW q: FP3
                  nach Einspielen von PJ29457: drucken Åber Netz geht wieder
---  ----------------  -----  ---------  --------- -----  ------------------
                             2763026626  289424712   90%  7 chapters

You can see the list of five chapters with the most important chapter data. On
the right you can see the name of the chapter, in the lines between there is
an (optional) comment for the archive. It is obvious that the first archive
has a similar compression to Zip, while the other chapters become better and
better compressed.

The complete archive file has a size of 276 MB and contains the full backups
of five OS/2 partitions of two PCs, which include 1761 MB in total. This
results in a compression rate of 84%; if you would zip these five full
backups, you would need 3,5 times of the size of the NDB backups!

And similar to Zip you can handle each file of each archive individually,
because all files and directories are not archived as one big image, but as
individual files and directories internally.

This was the reason for the development of NDB: If there are similar data (or
the same data more often) to backup, you can save much space, if the archive
program saves only the differences (and in addition compresses the
differences).

A nightly backup, that saves most recent data files (mails, office documents)
with NDB 0.2.x, has been running on one of my PCs over the last 120 nights. In
total it archived about 120 x 120 MB = 14,4 GB. The archive file needs only
177 MB, in other words the size of two zip files is filled with 120 full
backups.


How does NDB work?

NDB splits every file that goes into the archive into blocks and checks for
all blocks, whether its content is already included in the archive file. A new
full backup of already archived data requires much less space, because only
changed blocks (plus some administration data) are saved. Additionally, the
blocks are compressed before writing them to the archive using the zlib
library, that uses a compression algorithm compatible to the one used by the
well-known Zip (InfoZip, WinZip, etc) tools.

NDB's compression is not as good as Zip, Rar, Arj etc on the first full
backup, as it needs extra space in the archive for administrative information.
Also, NDB only compresses single blocks but never large parts of data.

But as soon as files are there twice in the data or the user runs a second (or
third ...) backup of the files, the compression ratio achieved by NDB is
better than that of other tools.


2. OS/2, Windows and Linux
==========================

OS/2:

The only difficulty with OS/2 are the extended attributes (EAs). But because
the sources of Info-Zip's zip & unzip are freely available, NDB also can
archive and extract the EAs. As a test I archived my maintenance partition,
made a long format of it and extracted the data again - and OS/2 started
again, as if nothing had happen. Therefore NDB is a possible disaster recovery
tool, if you have the possibility to boot from another source with an
installed emx runtime. I use NDB now every time before I change anything in
the OS/2 or eCS configuration and before I install anything.

To use NDB with OS/2 you need the four NDB executables, the zlib Dll
(z.dll) and - until 0.6.4 - the emx runtime 0.9d. From 0.6.5 NDB needs
the Innotek runtime, but can handle files over 2 GB file size.


Win32:

Win32 has one difficulty for each backup tool: automagically created
shortnames for each filename exceeding the 8.3 length. Because the shortnames
are created by Win32 itself, a backup tool cannot guarantee (*) to have the same
8.3 names after restoring. Sadly the registry is filled up with 8.3 names. So
a disaster recovery from a Zip, Rar, Arj, NDB or similar archive file may end
in a disaster. Real disaster recovery tools mostly create the correct 8.3
names by saving sector by sector in a huge image file, not by copying file by
file. The security descriptors of NTFS (usable by NT/W2K/XP) are implemented.

(*): Since WinXP there is an API call which allows the setting of an shortname
for NTFS drives. In all other combinations (Win9X, ME, Win2K; FAT32, FAT16)
NDB is able to save the shortnames but not to restore them again. A desaster
recovery therefore is only possible for XP/NTFS. To avoid failures because of
locked files you must boot from a bootable XP CD (a howto can be found at
http://www.nu2.nu/pebuilder/) or a second XP installation on a different
partition.


Linux:

Since 0.4.2 there is a Linux version, which allows to archive and
extract directories and 'normal' files. The special unix attributes group ID
and user ID and the rwx privilege groups are also saved and restored. Since
0.4.6 symbolic links can be archived and restored as well; before, the linked
file itself was archived and restored.



3. Examples/Quick Guide
=======================


Prerequisite for the examples below are batch files or aliases
(OS/2: ndbc.cmd, ndba.cmd, ndbe.cmd, ndbl.cmd; Win32: ndbc.bat, ndba.bat,
ndbe.bat, ndbl.bat, Linux: links or aliases for all four executables)
to correctly point to the executables.


ndbc d:\backup\test.ndb
	create the NDB archive file

ndba d:\backup\test.ndb -n *.pap
	archive all data files of the Papyrus office suite

ndba d:\backup\test.ndb -n *.pap
	make another backup of all data files of the Papyrus office suite

ndbl d:\backup\test.ndb
	list all chapters (=backups) within the NDB archive file

ndbl d:\backup\test.ndb -c0
	show all backed up files from the first chapter (= no. 0)

ndbl d:\backup\test.ndb -c1
	show all backed up files from the second chapter (= no. 1)

ndbl d:\backup\test.ndb -c-0 -lc
	show all changed (-lc) files from the newest backup (-c-<no>
	counts backwards)

ndbe d:\backup\test.ndb -c0 *
	extract all files from the first chapter

ndbe d:\backup\test.ndb -a *
	extract all files from all chapters


To give a more advanced example, the development sources of NDB are
thought to be contained in an NDB archive:

ndbc ..\projekt_ndb.ndb
	create the archive file first

ndba ..\projekt_ndb.ndb -rs -n"NDB V0.0.0" * -x *.exe *.o *.map
	archive the initial state
ndba ..\projekt_ndb.ndb -rs -n"NDB V0.0.1" "*" -x "*.exe" "*.o" "*.map"
	archive the next development cycle

...     etc.

ndba ..\projekt_ndb.ndb -rs -n"NDB V0.1.10" * -x *.exe *.o *.map
	archive version 0.1.10

ndba ..\projekt_ndb.ndb -rs -l *.c *.h -m"Correction of NDB V0.1.10"
	add another patch into the last chapter

The format of the NDB archive has changed during development:

ndbe ..\projekt_ndb.ndb -a *
	unpack all chapters (with the old version of ndbextract!)

ndba.cmd (or ndba.bat)
	edit it to point to the new executable
ndb_importall.cmd (or .bat)
	repack all chapters again



4. Tips
=======


Tip: A sensible automatic backup could be set up like this:
- First, the NDB archive file is created with the option  -f<size>. As
filesize for the data files the user might choose CD size (-f665M). (If
a DVD writer is available the size can of course be increased.)
- NDB is started by Cron (or a similar task planner/scheduler) and is
activated once a day automatically. Thanks to the parameter -cf the time
the backup takes will be short.
- Depending on the needed security, the main NDB archive file and the
newly created data files can be burned on CD/DVD once a week or once
a month. As soon as a new data file is created, the older data files
will only be read from but not changed. These therefore do not need to
be written on CD again. (You can do this of course if you do not trust
the quality of your CD blanks. ;-) )

Tip: With version 0.4.x the archive can be splitted in one main file
containing control data one several data files. This way one can archive
more than 2 or 4 GB while the single archive files can still fit onto
CD blanks.
However, the choice if and how to split up the archive files is only
available when creating the main archive file with NDBC! Afterwards,
NDB cannot change this any more or you have to unpack all chapters and
repack then into a newly created NDB archive, giving the new size when
running NDBC.

Tip: Ongoing archiving with NDB can be stopped without any serious
consequences by pressing CTRL-C as long as the message "updating archive
inventory" (that appears after archiving all files but before changes
in the administration data) has not appeared. Only after this message
is the new chapter added to the administration data. If the program
is stopped before that the archive file will already have grown as
compressed binary data has been written to it, but a new archiving run
will seamlessly start to write data where it was stopped before.



5. Usage (Complete Guide)
=========================


ndbcreate.exe

creates an empty NDB archive file and fixes the block size and the compression
level for all chapters; if there is a data file size handed over, only the
maintenance data is written in the NDB archive size, the packed binary data of
the archived files is saved in data files, which won't exceed the given
file size


usage: ndbcreate <archive[.ndb]> [-o] [-b<blocksize>] [-l<compressionlevel>]
       -o:              *o*verwrite existing file without any question
       -b<size>:        *b*lock size, must be between 1024 and 65535
       -f<size>:        data*f*ile size, must be between 64k and 2g
       -l<level>:       use zlib compression *l*evel <level>
       -m"<comment>":   add co*m*ment (max. 79 chars)
       -d<level>:       set *d*ebug level 1..9

    -o
        creates a new <archive.ndb> without any question also if there is a
        file with the same name

    -b<size>
        default is '-b60k' for blocks with 60 kbytes, if nothing given;
        fixes the block size, the archived files get devided internally;
        it must be between 1024 and 65535 bytes; 'k' kan be used for kbytes;
        normally bigger is better (better compression, less control data
        inside NDB)

    -f<size>
        NEW: since NDB V0.4.x.
        with '-f' the archive file is diveded in one control file, which
        contains the meta and controll data) and several data files, which
        contain the compressed binary data of the archived files; with
        this mechanism the 4 GB limit for the whole NDB archive can be
        broken
        With "-f<size>" you tell NDB, how much a data file can grow,
        until a new one must be created: <size> is a number (bytes),
        a trailing 'k' for kbytes or 'm' for mbytes may follow;
        Using "-f650m" you can burn all parts on CD, "-f1440k" allows
        the splitting for HD floppies ;-)

    -l<level>
        default is '-l5' if nothing given;
        fixes the compression level between 0 (no compression) and
        9 (highest compression); between 5 and 9 there is nearly no size
        difference but a noticeable CPU load difference

    -m"<comment>"
        adds a comment to the NDB archive

    -d<level>
        default is '-d0' if nothing given
        set the debug level from 0 to 9
            with 1 additional, "general" infomations
            with 2 more "general" infomations
            with 3 first technical info (source details)
            with 5 most functions tell start and finish, only smaller
                   functions within loops are still quiet
            with 6 nearly all functions tell start and finish
            with 7 all functions, many detail data of internal structures
                   (chapters, file header, block header, ...)
            with 8 like 7, additionaly the written or read bytes of each
                   structure
            with 9 like 8, additionaly all zipped and unzipped data blocks
                   are dumped

        with level 3/5/7/9 the output grows clearly, using 7 or more
               will create a *real* big log file



ndbarchive.exe

saves the files (selected by a file mask) into the NDB archive file;
the NDB archive file must exist already, because ndbarchive does not
create it


usage:
ndbarchive <archiv> -n<name> [options] [-d<level>] [-w<millis>] <files>
                    [-x|i <files>] [-i@<file>] [-x@<file>]
    List of all parameters and options:
    -h:           this help text
    <archiv>:     name of NDB archive file
    <files>:      filename(s), wildcards are * and ?
                  (use double quotes for unix)
    -n[<"name">]: create a *n*ew chapter with name <"name">
    -l:           add files to *l*ast chapter
    -r:           *r*ecurse into subdirectories (don't archive empty dirs)
    -R:           like -r, archive empty dirs also
    -s:           add hidden and *s*ystem files
    -cx:          *c*ompare files e*x*actly (default)
                  (slowest speed, exact test)
    -cm:          *c*ompare files by their *m*d5
                  (medium speed, exact test)
    -cc:          *c*ompare files by their block *c*rcs
                  (faster than -cm, but risk of 1:4294967296
                   to miss a change in the file)
    -cf:          *c*ompare files *f*ast
                  (lightning speed, exact test if all OS conform)
    -X:           ignore e*X*tra data (e.g. OS/2 EAs)
    -x <files>:   e*x*clude following files
    -i <files>:   *i*nclude following files
    -x@<file>:    e*x*clude file(mask)s listed in <file>
    -i@<file>:    *i*nclude file(mask)s listed in <file>
    -t<yyyymmddhhmmss>  set chapter creation *t*ime
    -m<text>:     add a co*m*ment (max 79 chars) for this chapter
    -d<level>:    set *d*ebug level 1..9
    -w<millis>:   *w*ait <millis> milliseconds after each file


    <archiv>
    	filename of the NDB archive file; if the name doesn't end with
    	".ndb", ".ndb" will be added

    <files>
    	file masks, use * and ? as wildcards;
		if you type an existing directory name - without wildcards -
		NDB automatically adds "\*" to it;
		if you type an existing file name - without wildcards -
		NDB ignores the flags '-r' or '-R' for this name
    	[Linux: masks with wildcards must be surrounded with ", to bypass
    	the shell]

    -n[<"name">]
        NDB creates a new chapter for this archive run;
        if you don't submit a chapter name, NDB uses
        "BAK YYYY-MM-DD hh:mm" as chapter name

    -r
        proceed all subdirectories also; without this flag all directories
        are skipped
        empty directories are not saved (use -R instead)

    -R
        like -r, but saved empty subdirectories also

    -s
        saves files with hidden and system flags also

    -c
        -cx: default: always check the complete file content against the NDB
             archive file
             -> safe but slow
        -cf: if the previous chapter contains a file with the same attributes
             (path, name, creation time, modification time, size, file
             attributes), then this file isn't processed, then NDB assumes
             that the current file is identical to the previously archived
             file and only copies the control data from the previous chapter
             -> much quicker; normaly safe also - only if you use disk
             editors or 'bad' software which restores all file attributes
             after manipulating a file, then changes are not detected
    	-cc: for each file to archive the CRC32 values of its blocks are
    	     calculated and compared with those of the file with the same
    	     name from the previous chapter; if both files sizes and all
    	     CRC32 values are identical, identity is assumed and only
    	     the pointers to the block data are copied to the new file.
             -> faster than -cx, slower than -cf; with a risk of
             1 : 4.294.967.296 a change in a block gets not noticed!
    	-cm: for each file to archive its MD5 value is calculated and
    	     compared with the MD5 of the file with the same name from
    	     the previous chapter; if both files sizes and both MD5 values
    	     are identical, identity is assumed and only the pointers to
    	     the block data are copied to the new file.
             -> faster than -cx, a little bit slower than -cc, slower than
             -cf

    -X
        don't archive "extra" data (e.g. EAs of OS/2 or NTFS security
        descriptors); default is saving all extra data

    -y  (only Unix, since 0.4.6) normaly NDB doesn't save links, but the
        files, the links point to; with '-y' NDB saves links as links

    -x
        exclude files you don't want to save (e.g. ndba ... * -x *.bak,
        save everything but exclude all backups)

    -i
        include files, used after '-x'
        (e.g. ndba ... * -x *.bak -i *important*.bak, save everything but
        exclude all backups except the important backups)

    -x@<file>
    	<file> has to contain a list of filemasks, each in a seperate line;
    	each filemask is added to the exclude list.

    -i@<file>
    	<file> has to contain a list of filemasks, each in a seperate
    	line; each filemask is added to the list of filemasks which will
    	be archived.

    -t<yyyymmddhhmmss>
        internal command, used for the import/export
        (look for ndbextract.exe -a)
        with '-t' you can change the timepoint, NDB saves as creation time
        for the chapter; useful if you want to import chapters from other
        NDB archives and want to keep the original timestamp

    -m<text>
        add a comment for this chapter

    -d<level>
        debug levels; see ndbcreate -d

    -w<msec>
        default '-w0' if nothing given
        wait <msec> milliseconds after each archived file; archiving
        and extracting may need so much I/O, CPU or net band-width,
        that you can't use your pc during this time;
        use something like '-w25' or '-w100' if you want to continue
        working



ndbextract.exe

extracts data from the archive file; using file masks you can selected
which files have to be extracted; if a file already exists on disk, NDB
ask what to do (using '-o' you can tell NDB to overwrite all files)


usage:
ndbextract <archiv> [options] -a|-c<number> <files>
    List of all parameters and options:
    -h:               this help text
    <archiv>:         name of NDB archive file
    <files>:          filename(s), wildcards are * and ?
                      (use double quotes for unix)
    -a[<from>-<to>]:  extract *a*ll chapters (restrict to <from> - <to>)
    -em:              *e*xtract only 'm'odified files
    -c<number>:       extract content from *c*hapter <number>
    -R:               *r*ecursivly extract empty dirs also
    -o:               *o*verwrite existing files without any question
      -oa:            overwrite *a*ll existing files
      -on:            overwrite *n*o existing files
      -om:            overwrite *m*odified files only
    -x:               e*x*clude following files
    -i:               *i*nclude following files
    -x@<file>:        e*x*clude file(mask)s listed in <file>
    -i@<file>:        *i*nclude file(mask)s listed in <file>
    -t:               *t*est archiv integrity (no extraction)
    -X:               ignore e*X*tra data (e.g. OS/2 EAs)
    -p<path>:         extract to <path>, not to current dir
    -d<level>:        set *d*ebug level 1..9
    -w<millis>:       *w*ait <millis> milliseconds after each file


    <archiv>
    	filename of the NDB archive file; if the name doesn't end with
    	".ndb", ".ndb" will be added

    <files>
    	file masks, use * and ? as wildcards;
		if you type an existing directory name - without wildcards -
		NDB automatically adds "\*" to it;
		if you type an existing file name - without wildcards -
		NDB ignores the flags '-r' or '-R' for this name
    	[Linux: masks with wildcards must be surrounded with ", to bypass
    	the shell]

    -c<number>
        extracts all files matching the file mask <files> from chapter
        <number>; while restoring NDB checks, that the CRC and original
        length of all blocks and the whole file size and CRC matches the
        original values;
        nice feature: -c-<number> counts backwards, therefore '-c-0' is
        always the newest chapter and so on

    -a
        extracts all chapters in subdirectories (named 'chap<number>')
        and creates a batch file which can import all chapters into a
        new NDB file; the batch file also sets the old values for
        chapter name, chapter creation time and chapter comment;
        because of extracting all chapters may need *very* much disk space,
        you can tell NDB to extract only some chapters: allowed are
        '-a<from>-<to>', '-a<from>-' and '-a-<to>'

    -em
        only files, which have changed against previous chapters, will
        be extracted

    -R
        using '-R' *all* saved directories of the selected chapter(s) are
        restored, no matter which file mask(s)  you use; in other words,
        the whole directory structure, which was saved into the chapter,
        is extracted completely; the files within this directory structure
        are selected by your file mask(s);
        currently there is no possibility to tell NDB to restore certain
        directories only, because the file mask(s) work(s) only for files

    -o
        while extracting NDB checks for already existing files and ask
        something like "Overwrite it: (y)es, (n)o, (m)odified only, (A)ll,
        (N)one?";
        with '-o' you can choose "(A)ll" as standard
    -oa
        same as '-o'; every file is extracted, no matter whether there
        is a file with the same name on your drive or not
    -on
        with '-on' you can choose "(N)one" as standard; every file which
        already exists on your drive is skipped
    -om
        with '-om' you can choose "(m)odified only" as standard;
        only missing and different files - NDB checks files size, file
        attributes, creation time and last modification time - are
        extracted/overwritten; this is much faster than extracting all
        files

    -x<filemask>
        don't extract files matching <filemask>

    -i<filemask>
        use after '-x<filemask>' to include some of the excluded files again

    -x@<file>
    	<file> has to contain a list of filemasks, each in a seperate line;
    	each filemask is added to the exclude list.

    -i@<file>
    	<file> has to contain a list of filemasks, each in a seperate
    	line; each filemask is added to the list of filemasks which will
    	be archived.

    -X
        don't extract extra data (OS/2: no EAs are restored, NT/W2K/XP:
        no security descriptors are restored)
        entpackt.

    -p<path>:
        extracts the selected files not within the current directory but
        in <path>

    -t
        test a NDB archive file; extract, but don't write anything to disk;
        NDB tells if CRC or size of any block or the complete files don't
        match the origial values

    -d<level>
        debug levels; see ndbcreate -d

    -w<msec>
        wait <msec> milliseconds after each file; see ndbarchive -w for
        details



ndblist.exe

lists the chapters of a NDB archive file or the files of a chapter


usage: ndblist <container> [-c<number>] [-l<x>] [-d<level>] [file mask]
       -c<number>:  list content from *c*hapter <number> <name>
       -l<..>:      *l*ist level:
         s:         *s*hort output (only chapter without -c<nr>)
         m:         *m*edium output (only chapter without -c<nr>) [default]
         l:         *l*ong output (only chapter without -c<nr>)
         c:         show *c*hanged files only
         a:         show chapter *a*llocations
         h:         show file *h*ash and name
         r:         *r*aw mode (usable for other programs)
         b:         show *b*lock list for each file (debugging)
       -d<level>:   set *d*ebug level 1..9

    <filemask>
        all files matching <filemask> are printed; without '-c<number>'
        all chapters are processed

    -c<number>
        lists the content of chapter <number>; without '-c<number>' all
        chapters are processed;
        nice feature: -c-<number> counts backwards, therefore '-c-0' is
        always the newest chapter and so on

    -l<..>
        select the output format (description is valid since 0.4.9)
        -lm
            (medium output, default) all files are printed (similar to
            'unzip -v'); no printing of extra data
            without '-c<number>' all chapters are listed but no files
            within the chapters (except you use <filemask>);
            if the chapters has a comment assigned, it is printed in a
            second line
        -ls
            (short output) without '-c<number>' and without <filemask>
            all chapters are listed (without chapter comments);
            without '-c<number>' but with <filemask> all files of all
            chapters are listed, the chapter number is only shown as
            4-digit-number in the front of each line; no extra data
        -ll
            (long output) similar to '.lm', each extra data part is
            printed as a seperate line
            without '-c<number>' all chapters are listed with NDB version
            and OS they were made and the comment, if there exists any;
            files are not listed
        -lc
            lists only files which are changed against the previous chapter;
            in combination with '-ls' and <filemask> you can see exactly,
            in which chapters files matching <filemask> have changed
        -lh
            list the hash value (CRC32 until 0.5.x, MD5 since 0.6.x) and the
            filename
        -la
            list the allocation of binary data, block control data and file
            control data for each chapter
        -lb
			-lb tells for every file of a chapter the whole block list with
			block number, status (new/identical), type (file data, extra
			data), CRC, original and packed size, number of data file, start
			position within the data file and finally the type of extra data
        -lr
            output in a special raw-format, which is more suitable for
            computer parsing than the normal human readable format;
            documentation currently only within source

    -d<level>
        debug levels; see ndbcreate -d



6. Limits
=========


The NDB programs were developed using normal C functions. Some of
the usual restrictions because of the 32bit address space result
from this:

- no more than 4 billion (10^9) files can be archived

until V0.5.x:
- all OSses: files of more than 2 GB size cannot be archived
since V0.6.2:
- Win32 & Linux: files with 2 GB or more can be archived
since V0.6.5:
- OS/2: files with 2 GB or more can be archived

until V0.3.x:
- all OSses: max. 2 GB archive size
since V0.4.x:
- all OSses: max. 2 GB archive size, additionally up to 65535 data files
- these data files themselves can be up to 2 GB in size

until V0.3.x:
- the original size of all archived files within each chapter can be
  larger than 2 GB as long as the sum of the compressed files stays
  below 2 GB (NTFS) or 2 GB (FAT32)
from V0.4.x on:
- the original file size and the size of all compressed files within
  each archive can now exceed file 4/2 GB

At the moment the following codepages are supported:
- iso8859-1
- iso8859-15
- us-ascii
- cp437
- cp850
- cp1004 (with restrictions, cp1252 is used internally)
- cp1252
- utf-8

If an unknown codepage is encountered, ndbarchive.exe saves the filename
without any conversion and adds a flag in the attributes for this file.
ndbextract.exe honors this flag and unpacks the file using the saved
filename without any conversion.


7. Technical Details
====================


Structure of the archive file:

- Archive chunk (structure ndb_s_c_archive)
- Chapter 0 (chapter chunk, binary data, list of block headers, list of files,
             list of data files if needed)
- Chapter 1 (chapter chunk, binary data, list of block headers, list of files,
             list of data files if needed)
- Chapter 2 (chapter chunk, binary data, list of block headers, list of files,
             list of data files if needed)
- ...
- Chapter n (chapter chunk, binary data, list of block headers, list of files,
             list of data files if needed)

Structure of each chapter:

- chapter chunk (structure ndb_s_c_chapter)
- binary data chunk (structure ndb_s_c_block)
- binary data (m zipped data blocks)
- chunk of block headers (structure ndb_s_c_block)
- m block headers (structure ndb_s_blockheader)
- chunk of file entries (structure ndb_s_c_block)
- n file entries (structure ndb_s_fileentry)
- 0..o data file header (structure ndb_s_datafileheader)

Archiving in blocks:

Each file is split up in blocks of fixed length (determined when calling
ndbcreate.exe). A CRC32 value is computed for each block and saved in
a hash table. Each block is then compressed and possibly saved in the
archive file.

Before a block is saved to the archive file the hash table is searched
for blocks with the same CRC32 value. If one is found that also has the
same properties (original length and zipped length), it is read from the
archive file and compared to the new block byte by byte. The new block
will only be written to the archive file, if differences are found.
Otherwise, only a pointer to the old block will be added to the list
of blocks.

The advantage of this method is that identical files or identical parts
of files only need additional space for the administrative information
(a block takes 28 bytes for that), resulting in a potentially very high
compression rate.

A disadvantage of this method is that splitting and administrating the
block-based information is more CPU intensive and that the compression
rate is slightly decreasing (this is especially true for smaller block
sizes).

One consequence from this is to use as large blocks as possible. First,
to decrease the administration effort (CPU load), and second, to achieve
good compression ratios. In the interface of ndbcreate a lower limit of
1024 byte is hard coded. It really only makes sense to use a multiple of
4096 bytes, as running time and archive size steeply increase below this
value. The default for the blocksize when calling ndbcreate is therefore
set to 60 kB.


Saving file metadata:

To be able to handle archive files independent from OS and codepage as
basic subset of metadata is held in the structure ndb_s_fileentry. OS
specific data (as OS/2's EAs or NTFS's security information) are saved
externally as "extra data". To be independent from the codepage the
filenames are converted into UTF-8 and then saved into the archive file.
The result of this are two restrictions: if NDB does not recognize the
codepage of the system, the filename can only be saved without conversion;
when extracting a file and the current codepage cannot deal with a
UTF-8 value, the file cannot be named correctly and contains all UTF-8
characters above 128 as hexadecimal encoding.


Space requirements of the archive file:

The chapter itself uses about 300 to 400 bytes with all its chunks.

The file entry structure uses 46 bytes plus bytes for the filename, a block
header structure contains 28 bytes. A ordinary compressible file (with ratio
of ~50%) of 1 MB size and a name of 40 characters will use about 500 kB of
compressed data and about 565 bytes of administrative information. (If this
file is still unchanged when archived again in another chapter later on, only
these 565 bytes of administrative information are used.)

Memory requirements in RAM:

NDB needs a relatively large amount of RAM to archive and extract as it
keeps all administrative information plus all necessary pointers, helper
lists, and hash tables completely in RAM all at the same time.

For the WINNT directory of W2K SP4 (8.237 Files, 185 Dirs, 973 MB) nearly 7 MB
of RAM were necessary, i.e. a mean of 860 bytes per file/directory.
To the compressed file data of 394 MB and additional 1.2 MB of administrative
information was needed on disk, i.e. a mean of of about 150 bytes per
file/directory.


8. Known Bugs
=============

all OS: NDB always assumes that the filesystem supports all the features
that are saved in the archive and that are associated with the current OS
such as long filenames, EAs, or NTFS security information.

Filenames are not tested for invalid characters depending on the filesystem.
(Different filesystems may allow different characters for filenames.)

No provision is taken for daylight saving time or different timezones.

No discrimination between NTFS, FAT32, and VFAT.

Generally: NDB is programmed relatively "defensive", but still e.g. zip &
unzip contain a lot more tests and security measures.


9. Planned Further Development
==============================

Graphical User Interface for mouse operation:

A few tries using Java+Swing exist, with a simple interface in explorer style
to browse through an archive file. This is a two-window-view, a tree based
view on the left, and details of the selected object on the right. Open
Archive -> List of Chapters, Open Chapter -> List of Files.

Implement further codepages.

A parameter to call the programs with a file containing codepage
information.


Perhaps sometime:

An additional executable ndbmaint.exe to manipulate the NDB archive, so that
one could cut a chapter or change the values (like name or comment) of a
chapter after creation of that chapter.
