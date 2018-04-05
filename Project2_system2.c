//Jonas Vinter-Jensen
//CSC 656[01], San Francisco State University, Spring-18

/*How to compile and run this program:
1)
Navigate to the unzipped folder with the .c files, submitted to iLearn, with the terminal

2)
Build sys2 executable:
gcc -o sys2 system2.c

3)
Set executable file-mode for all users:
chmod a+x sys2

4)
Upload the sys2 executable to some folder on Unixlab with the scp command

5)
Navigate to the folder containing the uploaded sys2 executable with terminal and run the following command:
./sys2 ~whsu/csc656/Traces/S18/P1/sjeng.xac -v
 */

#include <inttypes.h> //strtol()
#include <limits.h> //strtol(), required for legacy programs
#include <stdarg.h> //va_list, va_start
#include <stdint.h> //uint64_t
#include <stdio.h> //fscanf(), vprintf()
#include <stdlib.h> //EXIT
#include <string.h> //strcmp()
#include <math.h> //log2()

double branchMispredictionRate = 0.0;
FILE* fp;
int backwardBranches = 0;
int backwardTakenBranches = 0;
int branchOrder = 0;
int BTB_accesses = 0;
int BTB_misses = 0;
int current_PSTATE = 0;
int forwardBranches = 0;
int forwardTakenBranches = 0;
int logM = 0; //log2(M) bits required to locate BTB array entry
int logN = 0; //Log2(N) bits required to locate a prediction Buffer array entry
int M = 0; //Number of entries in the BTB array
int mispredictedBranches = 0;
int N = 0; //Number of entries in the 2-bit predictor array
int new_PSTATE = 0;
int prediction = 1; //Default: 01, this will be used to read the current prediction from the predictionBuffer
int totalBranches = 0;
int verboseState = 0; //Verbose mode off by default
uint64_t BTB_Index = 0; //logM bit index which locates the relevant BTB entry
uint64_t PC_tag = 0; //64-logM bits of PC, which will be compared to the tag of a given BTB entry
uint64_t predictorIndex = 0; //logN bit predictor index which locates the relevant Predictor Entry

uint64_t PC_Trace = 0; //Address of branch instruction, 64 bits, position 2 in trace
char Branch; //Single character indicating whether branch was taken, position 7
uint64_t TargetPC = 0; //BTA, 64 bits because of the Intel x86 ISA, position 12

//Each slot of the prediction Buffer array will contain a 2-bit predictor
struct predictionBuffer
{
    //2-bit predictor bit field
    unsigned int predictorBits : 2;
};

//Each slot of the BTB array will contain a BTB struct
struct BranchTargetBuffer
{
    int valid; //Invalid, until the given BTA entry has been accessed

    //If there are 2^64 addresses, because x86 Intel addresses are 64 bits, then
    //the maximum available bits for the index must be log2(2^64)-2 = 63. This is because
    //the tag, which is the remaining 1 bit, must at least be 1 bit long. Since the last
    //two bits are not 00, due to the ISA not being word-aligned, this leaves a maximum
    //value of 63 bits for the index. For this reason, index must have the datatype int64_t, since
    //it must be able to contain 63 bits. Unsigned because
    //literal addresses are absolute, not relative (like a backwards branch jump).
    int64_t index;

    //Same reasoning as for the index, with regards to the datatype. Unsigned for the
    //same reason that the index is.
    uint64_t tag;

    //64 bits, because the default size of a x86 address is 64 bits and unsigned for the
    //same reason that the index is.
    uint64_t BTA;
};

int verbose(const char* restrict, ...);
void setVerbose(int);

int main(int argc, char* argv[])
{
    //Open the trace file in read-only mode
    const char* filename;

    //Arguments: trace_filename N M [-v]
    if(argc==4 || argc==5)
    {
        filename = argv[1];
        fp = fopen(filename, "r");
        if(fp == NULL)
        {
            perror("File could not be found in the current working directory\nExiting...\n");
            exit(EXIT_FAILURE);
        }

        int result = strtol(argv[2], NULL, 10);
        if(result==0)
        {
            perror("Could not convert the 2nd argument to an integer");
            exit(EXIT_FAILURE);
        }
        N = result; //Convert argv[2] from string to a base 10 long integer
        if(N%2 != 0 && N>=2)
        {
            perror("N is not a power of 2");
            exit(EXIT_FAILURE);
        }

        result = strtol(argv[3], NULL, 10);
        if(result==0)
        {
            perror("Could not convert the 3rd argument to an integer");
            exit(EXIT_FAILURE);
        }
        M = result;
        if(M%2 != 0 && M>=2)
        {
            perror("M is not a power of 2");
            exit(EXIT_FAILURE);
        }

        //Initialize Predictor and the BTB arrays, now that the sizes are known
        struct predictionBuffer PredBuff[N];
        verbose("prediction Buffer Size = %d\n", N);
        for(int i=0; i<N; i++)
        {
            //Set initial predictor bits to 01 for all entries in the predictionBuffer
            PredBuff[i].predictorBits = 1;
        }

        verbose("BTB_Size = %d\n", M);
        struct BranchTargetBuffer BTB[M];
        for(int i=0; i<M; i++) //Set all valid bits to 0, until a BTB hit occurs for the given entry
        {
            BTB[i].valid = 0;
            BTB[i].BTA = 0;
            BTB[i].tag = 0;
        }

        //Turn on verbose mode if the optional 4th argument is -v
        if( argv[4] != NULL && strcmp(argv[4], "-v") == 0 )
        {
            setVerbose(1);
        }

        //Get branch statistics
        int currentLine = 1;
        verbose("The first Predictor index of PredArray = %d\n", PredBuff[0].predictorBits);

        double doubleN = (double) N;
        logN = (int) log2(doubleN); //Cast is okay, since N is divisible by 2

        double doubleM = (double) M;
        logM = (int) log2(doubleM);

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
                              &PC_Trace,
                              &Branch,
                              &TargetPC);

            if(scanLine==EOF)
            {
                break;
            }

            //Only 3 fscanf units will be passed as arguments: PC, Branch and TargetPC
            if(scanLine != 3)
            {
                printf("Had trouble with reading line %i of trace\nExiting...\n", currentLine);
                exit(EXIT_FAILURE);
            }

            if(Branch=='T' || Branch=='N')
            {
                totalBranches++;
                //printf ("PC_Trace of line %i = %" PRIx64 ".\n", currentLine, PC_Trace);

                predictorIndex = 0; //Reset before each loop, to avoid carrying over the previous result
                //Apply a mask to PC_Trace to read the rightmost logN bits for the Predictor array
                for(int i=0; i<logN; i++)
                {
                    //1) Create a mask, for e.g. 100 to get the 3rd bit
                    int mask = 1 << i;

                    //2) Apply the mask to PC_Trace, if 3rd bit is 1, then masked_PC = 100; else masked_PC = 000
                    int masked_PC = PC_Trace & mask;

                    //3) OR the ith bit with current result to work towards finding the logN bits Predictor Index
                    predictorIndex = predictorIndex | masked_PC;
                }

                //Get prediction for B_i
                prediction = PredBuff[predictorIndex].predictorBits;
                current_PSTATE = prediction; //For verbose mode

                //Check BTB entry for B_i
                BTB_Index = 0;
                //Find the rightmost logM bits of PC
                for(int i=0; i<logM; i++)
                {
                    int mask = 1 << i;
                    int masked_PC = PC_Trace & mask;
                    BTB_Index = BTB_Index | masked_PC;
                }

                PC_tag = 0;
                //Find the tag of the PC, located in the 64-logM upper bits of the current PC address
                PC_tag = PC_Trace << logM;

                //For each branch B_i
                     //If Bi is predicted taken
                //Check whether B_i is actually taken/not taken and if the prediction was T
                if(prediction==2 || prediction==3)
                {
                    //If valid bit of BTB entry == 1 && tag of BTB entry == tag of B_i
                    if(BTB[BTB_Index].valid==1 && BTB[BTB_Index].tag==PC_tag)
                    {
                        //BTB Hit
                        BTB_accesses++;
                    }
                    else //else, BTB miss
                    {
                        BTB_misses++;
                        BTB_accesses++; //A miss is counted as an access
                    }

                    //If prediction == actual behavior, prediction is correct
                    if(Branch=='T')
                    {
                        //Update prediction for B_i
                        if(PredBuff[predictorIndex].predictorBits<3)
                        {
                            PredBuff[predictorIndex].predictorBits++;
                            new_PSTATE = PredBuff[predictorIndex].predictorBits;
                        }
                        BTB[BTB_Index].BTA = TargetPC; //Only taken branches have BTA's entered in the BTB
                        BTB[BTB_Index].valid = 1; //Valid bit of BTB entry = 1
                        BTB[BTB_Index].tag = PC_tag; //Tag of BTB entry = tag of B_i
                    }
                    else if(Branch=='N') //else, prediction wrong
                    {
                        mispredictedBranches++;
                        //Update prediction for B_i
                        if(PredBuff[predictorIndex].predictorBits>0)
                        {
                            PredBuff[predictorIndex].predictorBits--;
                            new_PSTATE = PredBuff[predictorIndex].predictorBits;
                        }
                    }
                }
                else if(prediction==0 || prediction==1) //If the prediction was NT
                {
                    //Prediction was false
                    if(Branch=='T')
                    {
                        mispredictedBranches++;

                        //Update prediction for B_i
                        if(PredBuff[predictorIndex].predictorBits<3)
                        {
                            PredBuff[predictorIndex].predictorBits++;
                            new_PSTATE = PredBuff[predictorIndex].predictorBits;
                        }
                    }
                    else if(Branch=='N')
                    {
                        //Update prediction for B_i
                        if(PredBuff[predictorIndex].predictorBits>0)
                        {
                            PredBuff[predictorIndex].predictorBits--;
                            new_PSTATE = PredBuff[predictorIndex].predictorBits;
                        }
                    }
                }

                if(PC_Trace<TargetPC)
                {
                    forwardBranches++;
                    if(Branch=='T')
                    {
                        forwardTakenBranches++;
                    }
                }
                else if(PC_Trace>TargetPC)
                {
                    backwardBranches++;
                    if(Branch=='T')
                    {
                        backwardTakenBranches++;
                    }
                }

                //BranchOrder, predictorIndex, current_PSTATE, new_PSTATE, BTB_Index, PC_tag, BTB_accesses, BTB_misses
                verbose("%d %" PRIx64 " %d %d %d %" PRIx64 " %d %d\n", branchOrder, predictorIndex, current_PSTATE,
                        new_PSTATE, BTB_Index, PC_tag, BTB_accesses, BTB_misses);
                branchOrder++;
            }

            //verbose("Line %i of trace was scanned\n", currentLine);
            currentLine++;
        } //while end


    }
    else
    {
        printf("Must provide three or four arguments: filename N M [-v]\nExiting...\n");
        exit(EXIT_FAILURE);
    }

    printf("Number of branches = %i\n", totalBranches);
    printf("Number of forward branches = %i\n", forwardBranches);
    printf("Number of forward taken branches = %i\n", forwardTakenBranches);
    printf("Number of backward branches = %i\n", backwardBranches);
    printf("Number of backward taken branches = %i\n", backwardTakenBranches);

    if(totalBranches>0){branchMispredictionRate = (double) mispredictedBranches/totalBranches;}
    printf("Number of mispredictions = %i %f\n", mispredictedBranches, branchMispredictionRate);

    double BTB_miss_rate = 0.0;
    if(BTB_accesses>0){ BTB_miss_rate = (double) BTB_misses/BTB_accesses; }
    printf("Number of BTB misses = %d %f\n", BTB_misses, BTB_miss_rate);

    fclose(fp);
    return 0;
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
