#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <fstream>
#include "pin.H"

static UINT64 takenCorrect = 0;
static UINT64 takenIncorrect = 0;
static UINT64 notTakenCorrect = 0;
static UINT64 notTakenIncorrect = 0;

class BranchPredictor
{
    public:
        BranchPredictor() { }

        virtual bool makePrediction(ADDRINT address) { return false; };

        virtual void makeUpdate(bool takenActually, bool takenPredicted, ADDRINT address) {};

};

class myBranchPredictor: public BranchPredictor
{
    public:
	bool lh[1024][10];		//local history
	bool lp[1024][3];		//local predictor
	
	bool gh[12];			//global history
	bool cp[4096][2];		//choice predictor
	bool gp[4096][2];		//global predictor
	
	bool predictor;	
	int gi;					//global index
	

	// function that converts the bits into a decimal
	int bits2dec(bool *bits, int l){
		int decimal = 0;
		int base = 1;
		for(int i = 0; i < l; i++) {
			decimal = decimal + (base * bits[i]);
			base = base * 2;
		}
		return decimal;
	}

	// function that performs the prediction
	void pred(bool *bits, bool change, int l){
		int t;
		t=bits2dec(bits, l);
		
		if(l==2){	// 2 bit saturating counter
			if(change==1) {
				t++;
			} else {
				t--;
			}
			
			if (t == 0) {
				bits[0] = 0;
				bits[1] = 0;
				
			} else if (t == 1) {
				bits[0] = 1;
				bits[1] = 0;
				
			} else if (t == 2) {
				bits[0] = 0;
				bits[1] = 1;
				
			} else if (t == 3) { 
				bits[0] = 1;
				bits[1] = 1;
				
			}
		
		}

		else if(l==3){	// 3 bit saturating counter
            if(change==1) {
				t++;
			} else {
				t--;
			}

            if (t == 0) {
                bits[0]=0;
                bits[1]=0;
				bits[2]=0;
			} else if (t == 1) {
                bits[0]=1;
                bits[1]=0;
				bits[2]=0;
			} else if (t == 2) {
                bits[0]=0;
                bits[1]=1;
				bits[2]=0;
			} else if (t == 3) {
				bits[0]=1;
                bits[1]=1;
                bits[2]=0;
			} else if (t == 4) {
				bits[0]=0;
                bits[1]=0;
                bits[2]=1;
			} else if (t == 5) {
				bits[0]=1;
                bits[1]=0;
                bits[2]=1;
			} else if (t == 6) {
				bits[0]=0;
                bits[1]=1;
                bits[2]=1;
            } else if (t == 7) { 
				bits[0]=1;
				bits[1]=1;
                bits[2]=1;
			}
        }
			
							
	}
	// shifts every bit to the left by 1
	void bitshift1(bool *bits, bool a, int l){
		int i;
		for(i=1;i<l;i++){
			bits[i-1]=bits[i];	
		}	
		bits[l-1]=a; 
		
	}
	
    myBranchPredictor() {
		// initialize values for local history and local predictor
		for(int i = 0; i < 1024; i++) {
			for(int j = 0; j < 10; j++) {
				lh[i][j] = 0;
				
				lp[i][0] = 0;
				lp[i][1] = 0;
				lp[i][2] = 0;
			}
		}
		// initialize values for global and choice predictor
		for(int i = 0; i < 4096; i ++) {
			gp[i][0] = 0;
			gp[i][1] = 0;

			cp[i][0] = 0;
			cp[i][1] = 0;
		}

		for(int i = 0; i < 12; i++) {
			gh[i] = 0;
		}
	}

    bool makePrediction(ADDRINT address){ 
		
		long addr = (long) address;
		addr = addr%1024; 				// gets address - used to index into history table
		int index=bits2dec(gh, 12);
		gi=index;
		bool counter = cp[index][1];	// counter is index of choice predictor
		if (counter){ // local
			index=bits2dec(lh[addr],10);// gets index from local history - converts to decimal
			counter=lp[index][2];		// updates counter 
			predictor=0;				// predictor = 0 for local
		}
		else{ // global
			counter=gp[gi][1];			// updates counter 
			predictor=1;				// predictor = 1 for global
		}	

		return counter; 				// counter is returned - holds prediction
	}

    void makeUpdate(bool takenActually, bool takenPredicted, ADDRINT address){
		long addr=(long) address;
        addr = addr%1024;	// finds address, to be used to index into local history
		if(predictor) { // global
			pred(gp[gi],takenActually,2);		// gets entry for global predictor (2-bit)

			if(takenActually==takenPredicted) {
				pred(cp[gi], 0, 2);		// gets entry for choice predictor - histroy of global predictor (2-bit)
			} else {
				pred(cp[gi], 1, 2);
			}
		} else { // local
        	int index;

			index = bits2dec(lh[addr],10);	// gets index from local history and converts to decimal

			pred(lp[index], takenActually, 3);	// gets entry for local predictor (3-bit)
			if(takenActually==takenPredicted) {
				pred(cp[gi], 1, 2);		// gets entry for choice predictor - history of local predictor (2-bit)
			} else {
				pred(cp[gi], 0, 2);
			}
		}

		bitshift1(gh, takenActually, 12);		// shifts entries in global history

        bitshift1(lh[addr], takenActually, 10);	// shifts entriels in local history

	}

};

BranchPredictor* BP;


// This knob sets the output file name
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "result.out", "specify the output file name");


void handleBranch(ADDRINT ip, bool direction)
{
    bool prediction = BP->makePrediction(ip);
    BP->makeUpdate(direction, prediction, ip);


    if(prediction)
    {
        if(direction)
        {
            takenCorrect++;
        }
        else
        {
            takenIncorrect++;
        }
    }
    else
    {
        if(direction)
        {
            notTakenIncorrect++;
        }
        else
        {
            notTakenCorrect++;
        }
    }
}


void instrumentBranch(INS ins, void * v)
{   
    if(INS_IsBranch(ins) && INS_HasFallThrough(ins))
    {
        INS_InsertCall(
                ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)handleBranch,
                IARG_INST_PTR,
                IARG_BOOL,
                TRUE,
                IARG_END); 

        INS_InsertCall(
                ins, IPOINT_AFTER, (AFUNPTR)handleBranch,
                IARG_INST_PTR,
                IARG_BOOL,
                FALSE,
                IARG_END);
    }
}


/* ===================================================================== */
VOID Fini(int, VOID * v)
{   
	ofstream outfile;
	outfile.open(KnobOutputFile.Value().c_str());
	outfile.setf(ios::showbase);
	outfile << "takenCorrect: " << takenCorrect <<" takenIncorrect: "<< takenIncorrect <<" notTakenCorrect: "<< notTakenCorrect <<" notTakentIncorrect: "<< notTakenIncorrect << "\n";
	outfile.close();
/*    FILE* outfile;
    assert(outfile = fopen(KnobOutputFile.Value().c_str(),"w"));   
    fprintf(outfile, "takenCorrect %lu  takenIncorrect %lu notTakenCorrect %lu notTakenIncorrect %lu\n", takenCorrect, takenIncorrect, notTakenCorrect, notTakenIncorrect); */
}


// argc, argv are the entire command line, including pin -t <toolname> -- ...
int main(int argc, char * argv[])
{
    // Make a new branch predictor
    BP = new myBranchPredictor();

    // Initialize pin
    PIN_Init(argc, argv);

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(instrumentBranch, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}
