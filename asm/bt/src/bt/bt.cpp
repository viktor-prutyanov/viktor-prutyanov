/**
*   Binary translation
*
*   @file bt.cpp
*
*   @date 03.2015
*
*   @copyright GNU GPL v2.0
*
*   @author Viktor Prutyanov mailto:vitteran@gmail.com
*/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "Program.h"

#define CLR_RED     "\x1b[31m"
#define CLR_GRN     "\x1b[32m"
#define CLR_DFLT    "\x1b[0m"

Program *PROGRAM_PTR = nullptr; //Global pointer only for SIGSEGV handler

void SigSegv_action(int sig, siginfo_t *siginfo, void *ptr);

int main(int argc, char *argv[])
{
    bool arg_quiet = false, arg_dump = false, arg_noexec = false, arg_obj = false;
    char opt = 0;
    char *binFileName = nullptr, *objFileName = nullptr;

    while((opt = getopt(argc, argv, "dnqi:o:")) != -1)
    {
        switch(opt)
        {
        case 'd':
            arg_dump = true;
            break;
        case 'n':
            arg_noexec = true;
            break;
        case 'q':
            arg_quiet = true;
            break;
        case 'i':
            binFileName = optarg;
            break;
        case 'o':
            arg_obj = true;
            objFileName = optarg;
            break;
        default:
            puts("\
Binary translator from VPVM102 to x86_64 Linux\n\
Usage: bt [-dnv] [-i file] [-o file]\n\
 -d\tdump translation to 'bt_dump.txt'\n\
 -n\tnoexec mode (translate only)\n\
 -q\tquiet mode (errors only in output)\n\
 -o\toutput object file (for sl)\n\
 -i\tinput binary file");
            exit(EXIT_FAILURE);
            break;
        }
    }

    FILE *binFile = fopen(binFileName, "rb");
    if (binFile == nullptr)
    {
        printf("%s(BT_OPEN_ERROR)%s Invalid input file '%s'.\n", CLR_RED, CLR_DFLT, binFileName);
        return EXIT_FAILURE;
    }

    Program program(binFile, SigSegv_action);
    PROGRAM_PTR = &program;

    if (fclose(binFile) != 0)
    {
        printf("%s(BT_CLOSE_ERROR)%s Can't close input file '%s'.\n", 
            CLR_RED, CLR_DFLT, binFileName);
        return EXIT_FAILURE;
    }

    Command* lastCommand = program.Translate();

    FILE *dumpFile = nullptr;
    if (arg_dump)
    {
        dumpFile = fopen("bt_dump.txt", "w");
        if (dumpFile == nullptr)
        {
            printf("%s(BT_OPEN_ERROR)%s Can't open/create file 'bt_dump.txt'.\n", CLR_RED, CLR_DFLT);
            return EXIT_FAILURE;
        }   
    }

    if (lastCommand == nullptr)
    {
        if (!arg_quiet) printf("%s(BT_TRANSLATE_OK)%s Ready to exec.\n", CLR_GRN, CLR_DFLT);
        if (arg_dump) program.Dump(dumpFile);
    }
    else
    {
        printf("%s(BT_TRANSLATE_ERROR)%s Invalid command %d at %p in buf (try -d option).\n",
            CLR_RED, CLR_DFLT, lastCommand->num, program.BufPtr());
        if (arg_dump) 
        {
            program.Dump(dumpFile);
            if (fclose(dumpFile) != 0) printf("%s(BT_CLOSE_ERROR)%s Can't close file 'bt_dump.txt'.\n", 
                CLR_RED, CLR_DFLT);
        }
        return EXIT_FAILURE;
    }

    if (arg_dump && (fclose(dumpFile) != 0))
    {
        printf("%s(BT_CLOSE_ERROR)%s Can't close file 'bt_dump.txt'.\n", 
            CLR_RED, CLR_DFLT);
        return EXIT_FAILURE;
    }

    if (arg_obj)
    {
        FILE *objFile = fopen(objFileName, "wb");
        if (objFile == nullptr)
        {
            printf("%s(BT_OPEN_ERROR)%s Can't open/create obj file '%s'.\n", 
                CLR_RED, CLR_DFLT, objFileName);
            return EXIT_FAILURE;
        }
        program.GenerateObj(objFile);
        if (fclose(objFile) != 0)
        {
            printf("%s(BT_CLOSE_ERROR)%s Can't close obj file '%s'.\n", 
                CLR_RED, CLR_DFLT, objFileName);
            return EXIT_FAILURE;
        }
        printf("%s(BT_GEN_OBJ_OK)%s Object file '%s' created.\n", 
            CLR_GRN, CLR_DFLT, objFileName);
    }

    if (!arg_noexec) program.Exec();

    if (!arg_quiet && !arg_noexec) printf("%s(BT_EXEC_OK)%s Program exit.\n", CLR_GRN, CLR_DFLT);
    return EXIT_SUCCESS;
}

void SigSegv_action(int sig, siginfo_t *siginfo, void *ptr)
{
    printf("\n%s(BT_RUNTIME_ERROR)%s SIGSEGV (#%d) has captured at %p. Program exit.\n",
        CLR_RED, CLR_DFLT, sig, siginfo->si_addr);
    PROGRAM_PTR->~Program();
    exit(EXIT_FAILURE);
}
