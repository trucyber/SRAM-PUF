# Profiling Phase
A PUF consists of two unique components (1) a challenge, and (2) a response. In the SRAM context, a challenge represents the location of a cell, and the response represents the bit value stored in the location. The bit value of each cell can not be guaranteed throughout the life cycle of the SRAM. Due to the volatility of responses in SRAMs, ECC are needed to correct errors and generate stable responses. Through preliminary experiments, we found that the overhead of ECCs, although negligible for previous PUF protocols, have significant overhead in the SCADA context. The SPAI protocol avoid the overhead of ECCs by introducing a profiling phase that categorize the behavior of the SRAM. The profiling of the SRAM allows the prover to identify the strongest cells in the SRAM, thus eliminating the necessity of ECCs. We identify stable cells with a data remanence algorithm, which through 1,000 power-up tests, we found 2,000 stable cells of a 128Kb SRAM with both 1s and 0s values.

## Functions
1. verify_sram: test the proper operation of the SRAM
2. get_data_remanence: identify strong cells
3. identify_zeros: filter out strong zeros
4. identify_strong_zeros: identify strongest and stable zeros
5. identify_ones: filter out strong ones
6. identify_strong_ones: identify strongest and stable ones
7. write_strong_bit: write stables bits, the functions outputs the location of the strongest cell to a text file

## Compile and Run
make -f Makefile \
sudo ./getStrongBit