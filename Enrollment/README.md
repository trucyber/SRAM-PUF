# Enrollment Phase
In this step, the SPAI generates and stores a CRP required to authenticate the sensor. The steps are the following:
1) The verifier gets a challenge getChallenge(sBits, Ca).
2) The verifier appends the challenge CRDB:append(Ca).
3) Once the challenge Ca is generated, the verifier sends it through secure channels to the prover. During the enrollment phase, confidential information is exchanged, because of the critical nature of the information secure transmission channels are necessary.
4) Using the addresses in the challenge, the prover reads the bit value [0, 1] that is stored in the SRAM, creating a response Ra = getPUF(Ca).
5) The response must be kept secret if the attacker access this response. They can spoof the identity of the prover and eventually compromise the SCADA system.
6) Finally, the verifier appends the response to the CRDB using CRDB:append(Ra).

We consider that the SPAI protocol can repeat the enrollment phase as the storage capacity of the verifier allows. For demonstration, we use 2,000 stable cells for our evaluation, and set our challenge and response length to 256 bits.

## Functions
1. initialize: initilize all memory to temporally hold challenge and response
2. get_strong_bit: from the stable bits identified in the profiling phase, select a random sample
3. get_challenge: create a random 256 bit long challenge
4. get_response: get the PUF response of the random challenge

The enrollment phase outputs the challenge and response to both standard output and a temporally text file

## Compile and Run
make -f Makefile \
sudo ./enrollment