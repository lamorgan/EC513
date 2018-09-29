#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "pin.H"


// Output file name
ofstream OutFile;

// The array storing the spacing frequency between two dependant instructions
UINT64 *dependancySpacing;


UINT32 maxSize;

//arrays holding the registers and the instruction count
UINT32 inst_num = 0;
UINT32 writereg[400];	// picked size that seemed large enough
UINT32 readreg[40];

// This knob sets the output file name
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "result.csv", "specify the output file name");

// This knob will set the maximum spacing between two dependant instructions in the program
KNOB<string> KnobMaxSpacing(KNOB_MODE_WRITEONCE, "pintool", "s", "100", "specify the maximum spacing between two dependant instructions in the program");


// This function allocates the writereg array with the instruction count
VOID AllocateW(UINT32 cnt){
        writereg[cnt] = inst_num;
}

// This function is called before every instruction is executed. Have to change
// the code to send in the register names from the Instrumentation function
VOID updateSpacingInfo(UINT32 reg){
        UINT32 cnt;
	cnt = writereg[reg];
	// makes sure the index is within the maxSize
	// have to subtract 1 otherwise first index starts at 1 and not 0
	if(inst_num-cnt-1 < maxSize){ 
		dependancySpacing[inst_num-cnt-1]++; 
	}
}

// when a new instruction is encountered the count gets incremented
VOID new_instr() {
	inst_num++;
}


// Pin calls this function every time a new instruction is encountered
VOID Instruction(INS ins, VOID *v)
{
    // Insert a call to updateSpacingInfo before every instruction.
    // You may need to add arguments to the call.
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)new_instr, IARG_END);
	UINT32 reg;
	REG tmp;
	inst_num++;

	// loops through the write regs in instruction
	UINT32 max_w = INS_MaxNumWRegs(ins);
	for(UINT32 i = 0; i < max_w; i++){
		tmp = INS_RegW(ins, i);
		// if the reg is a partial, it gets the full reg name and sends it to allocate the writereg array
		if(REG_is_partialreg(tmp)){
			INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)AllocateW,IARG_UINT32, (UINT32)REG_FullRegName(tmp), IARG_END);
			INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)AllocateW,IARG_UINT32, (UINT32)tmp, IARG_END);	

		}
		// REG_EAX enum for the register we are looking at, easier to work with register
		// can't send tmp because its a register type not a UINT32
		// allocates the writereg array
		INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)AllocateW,IARG_UINT32, (UINT32)REG_EAX, IARG_END);
	}

	// loops through the read regs in instruction
	UINT32 max_r = INS_MaxNumRRegs(ins);
	UINT32 flag;
	for(UINT32 i = 0; i < max_r; i++){ 
                reg = (UINT32)INS_RegR(ins, i);
		flag = 0;
		// loops through read regs in the readreg array
                for(UINT32 j = 0;j < i; j++)
			// if the current register is the same as any previous register in the instruction flag is set to 1 (don't want to count
			// same register in same instruction twice)
                        if(readreg[j] == reg){
				flag = 1;
			}
                if(flag == 0) { 
			INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)updateSpacingInfo, IARG_UINT32, reg, IARG_END); // calls updateSpacingInfo
			readreg[i] = reg;	// allocates the readreg array
		}
	}

}

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{
    // Write to a file since cout and cerr maybe closed by the application
    OutFile.open(KnobOutputFile.Value().c_str());
    OutFile.setf(ios::showbase);
	for(UINT32 i = 0; i < maxSize; i++)
		OutFile << dependancySpacing[i]<<",";
    OutFile.close();
}


// argc, argv are the entire command line, including pin -t <toolname> -- ...
int main(int argc, char * argv[])
{
    // Initialize pin
    PIN_Init(argc, argv);

    printf("Warning: Pin Tool not implemented\n");

    maxSize = atoi(KnobMaxSpacing.Value().c_str());

    // Initializing depdendancy Spacing
    dependancySpacing = new UINT64[maxSize];

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}









