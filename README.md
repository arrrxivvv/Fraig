# Fraig
Logic Circuit Simplifier

## Introduction
This program simplies logic circuits by identifying and removing redundant gates. 
Circuit intput and output are done through circuit description .aag files, 
where all logic gates should already have been converted to and gates or inverter gates. 
Simplification is done by first performing simulation, then optimization. 

## Commands
CIRRead: read in a circuit and construct the netlist 
CIRPrint: print circuit 
CIRGate: report a gate 
CIRWrite: write the netlist to an ASCII AIG file (.aag) 
CIRSWeep: remove unused gates 
CIROPTimize: perform trivial optimizations 
CIRSTRash: perform structural hash on the circuit netlist 
CIRSIMulate: perform Boolean logic simulation on the circuit 
CIRFraig: perform FRAIG operation on the circuit


