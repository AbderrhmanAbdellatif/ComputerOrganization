LC2200-16 Tools Documentation

=> NOTE: This set of tools is intended to help you complete phase 2 of
project 1.

=> Introduction

The LC2200-16 is defined in the Project 1 description along with all its
specific implementation details in the provided datapath.  The tools in
this package are provided to assist in development and testing of your
LC2200-16 implementation. 

=> Example Files

Also included is a sample FSM input file which shows you how to code the first
two states of the FETCH operation.  You don't need to use this file as the
basis of your FSM code, but you may do so if you desire.

=> Loading Data into Logisim PROMs and RAMs

Once you have used the included tools to generate the code for your 
three ROMS and/or test
programs in the .hex format, you will need to load them into the correct 
places in your Logisim datapath. To load the file into either part, 
first, open up one of the generated HEX files (beqrom.txt, mainrom.txt, 
seqrom.txt, or converted assembly program), hit "ctrl+a" to highlight all the text and "ctrl+c" to copy 
it to the clipboard. Then, right click on the correct part in Logisim, 
select "Edit Contents..." from the menu, select ONLY the first byte in 
Logisim's HEX editor, and paste the copied HEX code into the ROM/RAM. 
Click "Save" to save your changes. The data is now loaded into the PROM/RAM module. 

=> MICROCompiler ROM Translator

The MICROCompiler ROM translator accepts as input a file specifying the various states
of the LC2200-16 state machine control logic along with the transitions between
states and the various control signals which should be asserted in each state.
As output, it provides a translated file containing a ROM/RAM image of the
state machine specified in the input file, suitable for loading into
Logisim as a hex input file to a PROM or RAM module.  Note that everything
in the MICROCompiler ROM Translator is cAsE sEnSiTiVe.  If you're not familiar with
what a grammar is or how to read one, simply skip down below the formal
grammar definition for a more extensive discussion of how to write compatible
files.  The grammar is present only so that if you know how to read one you
can quickly use it as a summary and avoid reading extra text.  Here is a
description of the grammar to use for your FSM input and some examples:

	The MICROCompiler takes in an XML file that contains a list of all the states, 
	the signals that must be asserted in each state, the next state that should be
	executed, and which state to go to for each outcome of the OnZ register. 
	Here is an example microstate (Note that this is not any real microstate, but
	rather just an example to help convey the grammar to you). 

		<Microcode>
			<State name="STATE0">
				<Signal name = "Signal1"/>
				<Signal name = "Signal2"/>
				<Goto state="STATE1"/>
			</State>
			<State name="STATE1" onZ="false">
				<Signal name = "Signal3"/>
				<Signal name = "Signal4"/>
				<Goto state="STATE2"/>
			</State>
		</Microcode>
	
	In order for MICO to work properly, you must wrap your entire microcode file with 
	the <Microcode> and </Microcode> tags. The next line: 
	
			<State name="STATE0" onZ="false">
	
	is the beginning of a new state description. Every state must include a name. As
	you can see in "STATE1", there is an optional onZ field that must be used on two 
	states. When a state has onZ="false", it is saying that this is the microstate that 
	should execute following the microstate where "chkZ" is asserted if zero detection 
	is false (i.e. the branch is not taken). Similarly, if a state has onZ="true" after
	it's name field, this is the state that should be executed follwing the "chkZ" signal
	assertion when zero detection is true (i.e. the branch is taken). Again, only two 
	states should contain the onZ="   " field, one of which should be "true" and one 
	should be "false". 
	
	The available state names are listed below: 
	
		  /*  0 */ "DUMMY",
		  /*  1 */ "FETCH0", "FETCH1", "FETCH2", "FETCH3", "FETCH4",
				   "FETCH5", "FETCH6", "FETCH7", "FETCH8", "FETCH9",
		  /* 11 */ "DECODE", //Decode is no longer needed
		  /* 12 */ "ADD0", "ADD1", "ADD2", "ADD3",
		  /* 16 */ "ADDI0", "ADDI1", "ADDI2", "ADDI3",
		  /* 20 */ "NAND0", "NAND1", "NAND2", "NAND3",
		  /* 24 */ "LW0", "LW1", "LW2", "LW3",
		  /* 28 */ "SW0", "SW1", "SW2", "SW3",
		  /* 32 */ "BEQ0", "BEQ1", "BEQ2", "BEQ3", "BEQ4",
				   "BEQ5", "BEQ6", "BEQ7", "BEQ8",
		  /* 41 */ "JALR0", "JALR1", "JALR2", "JALR3", "JALR4",
		  /* 55 */ "FUTURE", "FUTURE", "FUTURE", "FUTURE", "FUTURE",
				   "FUTURE", "FUTURE",
		  /* 62 */ "RESERVED",
		  /* 63 */ "HALT0"
			
	Next, between the <State> and </State> tags, we have our signal assertions as well 
	as our Goto statements, which is really our next state field. 
	
				<Signal name = "Signal3"/>
				<Goto state="STATE2"/>
	
	For each signal that must be asserted, a <Signal name="SigName"/> line must be used. 
	The signals that can be are listed below: 
		
		DrREG, DrMEM, DrALU, DrPC, DrOFF, LdPC, LdIR, LdMAR, LdA, LdB, LdZ, 
		WrREG, WrMEM, RegSelLo, RegSelHi, ALULo, ALUHi, OPTest, chkZ
	
To use the translator, enter the following at the console while you're in the
directory with the .jar and .xml files:

	java -jar MICROCompiler.jar -in <filename>.xml

This will produce output files called mainrom.txt, beqrom.txt, and seqrom.txt
which may then be loaded into the corresponding ROMs in your datapath. 

As an interesting note, the current optimal solution makes use of 28 states.
This is not to say that there does not exist a more optimal solution than
has yet been proven, but that's a pretty good lower bound.

=> Dealing with HALT

HALT is a special case. It's actually a pseudo-instruction translated into 
an SPOP (special operation), but you can ignore that.  All you need to ensure 
is that when a HALT operation is encountered no further instructions are 
fetched or run.  The single state HALT will be sufficient for this.  Be 
creative!

---
25 Jan. 2005: Initial revision, Kyle Goodwin
30 Jan. 2005: Revised for release, Kyle Goodwin
28 May  2005: Minor Touch-up, Matt Balaun
12 Sep. 2006: Revised for 16 bit, Kane Bonnette
19 May  2008: Cleaned up SPOP/HALT confusion, James Robinson
28 Aug. 2008: Added Windows binaries information and did some cleanup, Matt Bigelow
15 Sep. 2010: Re-written for the MICOCompiler three-ROM state machine. Assembler left untouched, Charlie Shilling
20 Jan. 2011: Split into several files: README-assembler.txt, README-standard.txt, and README-bonus.txt, Charlie Shilling
28 Aug 2013: Updated to main microcontroller for class, Kyle Kelly
10 May 2015: Recompiled MICOCompiler.jar -> MICROCompiler.jar