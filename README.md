# scvpi
Provides a stub implementation of (System)Verilog VPI functions for SystemC. 
Currently, this is just enough to allow cocotb to schedule coroutines. 
Access to signals, and other RTL-like things is not currently implemented. The purpose
of this package is to enable running a Python testbench with high-level SystemC
models.


## Using scvpi

- Compile scvpi with your SystemC model
    - Add <scvpi>/src/scvpi.cpp to your SystemC files
    - Add <scvpi>/src as an include path
    
- Specify VPI libraries to load at runtime
    - Add +vpi=<path_to_so> to the SystemC command line
    

