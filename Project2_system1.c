//Jonas Vinter-Jensen. 912941515
//CSC 656[01], San Francisco State University, Spring-18

/*How to compile and run this program:
1)
Navigate to the unzipped folder with the .c files, submitted to iLearn, with the terminal

2)
Build sys1 executable:
gcc -o sys1 system1.c

3)
Set executable file-mode for all users:
chmod a+x sys1

4)
Upload the sys2 executable to some folder on Unixlab with the scp command

5)
 Navigate to the folder containing the uploaded sys2 executable with terminal and run the following command:
./sys1 ~whsu/csc656/Traces/S18/P1/gcc.xac [-v]
 */

#include <inttypes.h>
#include <stdarg.h> //va_list, va_start
#include <stdint.h> //uint64_t
#include <stdio.h> //fscanf(), vprintf()
#include <stdlib.h> //EXIT
#include <string.h> //strcmp()

double branchMispredictionRate = 0.0;
int backwardBranches = 0;
int backwardTakenBranches = 0;
int forwardBranches = 0;
int forwardTakenBranches = 0;
int mispredictedBranches = 0;
int totalBranches = 0;
int verboseState = 0; //Verbose mode off by default

uint64_t PC = 0; //Address of branch instruction, 64 bits, position 2 in trace
char Branch = 0; //Single character indicating whether branch was taken, position 7
uint64_t TargetPC = 0; //BTA, 64 bits because of the Intel x86 ISA, position 12

int verbose(const char* restrict, ...);
void setVerbose(int);
void getBranchStatistics(FILE* fp);

int main(int argc, char* argv[])
{
    //Open the trace file in read-only mode
    const char* filename;
    FILE* fp;

    if(argc==2 || argc==3)
    {
        filename = argv[1];

        //Turn on verbose mode if the optional 2nd argument is -v
        if( argv[2] != NULL && strcmp(argv[2], "-v") == 0 )
        {
            setVerbose(1);
        }
    }
    else
    {
        printf("Must provide one or two arguments: filename [-v]\nExiting...\n");
        exit(EXIT_FAILURE);
    }

    fp = fopen(filename, "r");
    if(fp == NULL)
    {
        perror("File could not be found in the current working directory\nExiting...\n");
        exit(EXIT_FAILURE);
    }

    getBranchStatistics(fp);

    fclose(fp);
    return 0;
}

//Count the branches of the specified trace file
void getBranchStatistics(FILE* fp)
{
    int currentLine = 1;

    while(1)
    {
        //SCNixN: fscanf specifier for intN_t, %*: ignore the column with the following format
        int scanLine;
        scanLine = fscanf(fp,
                "%*" SCNi32
                "%" SCNx64
                "%*" SCNi32
                "%*" SCNi32
                "%*" SCNi32
                " %*c"
                " %c"
                " %*c"
                "%*" SCNi64
                "%*" SCNx64
                "%*" SCNx64
                "%" SCNx64
                "%*11s"
                "%*22s",
                          &PC,
                          &Branch,
                          &TargetPC);

        if(scanLine==EOF)
        {
            break;
        }

        if(scanLine != 3)
        {
            printf("Had trouble with reading line %i of trace\nExiting...\n", currentLine);
            exit(EXIT_FAILURE);
        }


        if(Branch=='T' || Branch=='N')
        {
            totalBranches++;

            if(PC<TargetPC)
            {
                forwardBranches++;
                if(Branch=='T')
                {
                    forwardTakenBranches++;
                    mispredictedBranches++; //Forward branches are predicted not taken
                }
            }
            else if(PC>TargetPC)
            {
                backwardBranches++;
                if(Branch=='T')
                {
                    backwardTakenBranches++;
                }
                else if(Branch=='N')
                {
                    mispredictedBranches++; //Backward branches are predicted taken
                }
            }
        }

        verbose("Line %i of trace was scanned\n", currentLine);
        currentLine++;
    } //while end

    printf("Number of branches = %i\n", totalBranches);
    printf("Number of forward branches = %i\n", forwardBranches);
    printf("Number of forward taken branches = %i\n", forwardTakenBranches);
    printf("Number of backward branches = %i\n", backwardBranches);
    printf("Number of backward taken branches = %i\n", backwardTakenBranches);

    if(totalBranches>0){branchMispredictionRate = (double) mispredictedBranches/totalBranches;}
    printf("Number of mispredictions = %i %f\n", mispredictedBranches, branchMispredictionRate);

    return;
}

void setVerbose(int state) //Call this from main.c to turn on/off verbose mode
{
    verboseState = state;
    return;
}

//Print this, only if verbose mode is active (verboseState == 1)
int verbose(const char* format, ...)
{
    if(!verboseState)
    {
        return 0;
    }

    va_list arglist;

    //Initialize arglist with the variable number of arguments in ... and format
    va_start(arglist, format);

    //Type of printf which can accept a list of arguments of type va_list
    int result = vprintf(format, arglist);

    //Prevent arglist from being called again, after all arguments have been used
    va_end(arglist);

    return result;
}
