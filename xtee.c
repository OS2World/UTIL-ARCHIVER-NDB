/*
** Das Programm kopiert die Eingaben von stdin auf stdout
** und protokolliert sie in allen angegebenen Dateien mit.
*/

#include <stdio.h>
#ifdef __MINGW32__
#include <io.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define PORTION 4096

int main(int argc, char *argv[])
{
   unsigned i;
   unsigned iCountFiles = 0;
   char currfilename[300];
   int opt_maxfiles = 0;
   unsigned long opt_datafilesize = 0L;
   char c;
   char *p;
   off_t filepos;
   FILE **stream = (FILE **)malloc((argc-1) * sizeof(FILE *));
   int *iFileNo = calloc(argc - 1, sizeof(int));
   char **filename = calloc(argc - 1, sizeof(char *));

   if (stream == NULL)
   {
      fprintf(stderr, "%s: kein Speicher\n", argv[0]);
      return EXIT_FAILURE;
   }

   if (argc == 1)
   {
      fprintf(stderr, "xtee: usage of %s:\n", argv[0]);
      fprintf(stderr, "xtee: <program> | xtee [-c<maxfiles>] [-f<maxfilesize>] file1 [file2 ...]\n", argv[0]);
      return EXIT_FAILURE;
   }

   for (i = 1; i < argc; ++i)
   {
        if (argv[i][0] != '-')
        {
          iCountFiles++;

          filename[iCountFiles-1] = calloc(300, 1);
          strcpy(filename[iCountFiles-1], argv[i]);

          sprintf (currfilename, "%s_%04d", filename[iCountFiles-1], iFileNo[i-1]);
          stream[iCountFiles-1] = fopen(currfilename, "wb");
          if (stream[iCountFiles-1] == NULL)
             perror(currfilename);
        }
        else
        {
            p = (char *) argv[i];
            if ((*p) == '-')
            {
                p++;

                c = *p;
                if (c == 'c')
                {
                    opt_maxfiles = atoi(++p);
                }

                else if (c == 'f')
                {
                    opt_datafilesize = atoi(++p);
                    while (isdigit((int) *p))
                    	p++;
                    if ((*p == 'k') || (*p == 'K'))
                    	opt_datafilesize *= 1024L;
                    if ((*p == 'm') || (*p == 'M'))
                    	opt_datafilesize *= 1024L * 1024L;
                    if ((*p == 'g') || (*p == 'G'))
                    	opt_datafilesize *= 1024L * 1024L * 1024L;
                    if ((opt_datafilesize != 0L) && (opt_datafilesize < PORTION))
                    {
                        fprintf(stdout, "xtee: Error: datafile size may not less than %d bytes.\n", PORTION);
                        fprintf(stdout, "xtee: Therefore setting datafile size to minimum of 1k.\n\n");
                        opt_datafilesize = PORTION;

                    }
                    if ((opt_datafilesize != 0L) && (opt_datafilesize >= 2147483647L))
                    {
                        fprintf(stdout, "xtee: Error: datafile size may not exceed 2GB.\n");
                        fprintf(stdout, "xtee: Therefore setting datafile size to maximun of 2147.483.647 bytes.\n\n");
                        opt_datafilesize = 2147483647L;
                    }
                }
                else
                {
                    fprintf(stdout, "xtee: unknown option -%s\n", p);
                }
            }
        }
   }

   // check options
   if ((opt_maxfiles > 0) && (opt_datafilesize == 0))
   {
      fprintf(stderr, "xtee: you need -f<maxsize> also if you want to use -c%d\n", opt_maxfiles);
      return EXIT_FAILURE;
   }

   do
   {
      char buffer[PORTION];

      size_t n = fread(buffer, 1, PORTION, stdin);

      fwrite(buffer, 1, n, stdout);

      for (i = 0; i < iCountFiles; ++i)
      {
         if (stream[i] != NULL)
         {
            // maximum file size reached?
            if (filepos == ftell(stream[i]))
            {
                if ((opt_datafilesize > PORTION) && ((filepos + n) > opt_datafilesize))
                {
                    // close file and open a new one
                    fclose(stream[i]);
                    sprintf (currfilename, "%s_%04d", filename[i], ++iFileNo[i]);
                    stream[i] = fopen(currfilename, "w");
                    if (stream[i] == NULL)
                       perror(currfilename);
                }
            }

            // do we need to delete old files?
            if ((opt_maxfiles > 0) && (iFileNo[i]) >= opt_maxfiles)
            {
                sprintf (currfilename, "%s_%04d", filename[i], iFileNo[i] - opt_maxfiles);
                unlink(currfilename);
            }

            // now, write input data to file finally
            n = fwrite(buffer, 1, n, stream[i]);
            fflush(stream[i]);
         }
      }
   }
   while (!feof(stdin) && !ferror(stdin));

   for (i = 0; i < iCountFiles; ++i)
   {
      if (stream[i] != NULL)
      {
         fclose(stream[i]);
      }
   }

   return EXIT_SUCCESS;
}
