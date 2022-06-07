# FUNCTIONAL DESCRIPTION
# Read Mode
Like conventional UVEPROMs, the W27C512 has two control functions, both of which produce data
at the outputs. CE is for power control and chip select. OE/VPP controls the output buffer to gate data
to the output pins. When addresses are stable, the address access time (TACC) is equal to the delay
from CE to output (TCE), and data are available at the outputs TOE after the falling edge of OE/VPP,
if TACC and TCE timings are met.

# Erase Mode
The erase operation is the only way to change data from "0" to "1." Unlike conventional UVEPROMs,
which use ultraviolet light to erase the contents of the entire chip (a procedure that requires up to half
an hour), the W27C512 uses electrical erasure. Generally, the chip can be erased within 100 mS by
using an EPROM writer with a special erase algorithm.
Erase mode is entered when OE/VPP is raised to VPE (14V), VCC = VCE (5V), A9 = VPE (14V), A0
low, and all other address pins low and data input pins high. Pulsing CE low starts the erase
operation.

# Erase Verify Mode
After an erase operation, all of the bytes in the chip must be verified to check whether they have been
successfully erased to "1" or not. The erase verify mode ensures a substantial erase margin if VCC =
VCE (3.75V), CE low, and OE/VPP low.

# Program Mode
Programming is performed exactly as it is in conventional UVEPROMs, and programming is the only
way to change cell data from "1" to "0." The program mode is entered when OE/VPP is raised to VPP
(12V), VCC = VCP (5V), the address pins equal the desired addresses, and the input pins equal the
desired inputs. Pulsing CE low starts the programming operation.

# Program Verify Mode
All of the bytes in the chip must be verified to check whether they have been successfully
programmed with the desired data or not. Hence, after each byte is programmed, a program verify
operation should be performed. The program verify mode automatically ensures a substantial
program margin. This mode will be entered after the program operation if OE/VPP low and CE low.

# Erase/Program Inhibit
Erase or program inhibit mode allows parallel erasing or programming of multiple chips with different
data. When CE high, erasing or programming of non-target chips is inhibited, so that except for the
CE and OE/VPP pins, the W27C512 may have common inputs.


# Standby Mode
The standby mode significantly reduces VCC current. This mode is entered when CE high. In standby
mode, all outputs are in a high impedance state, independent of OE/VPP.


# TABLE OF OPERATING MODES
(VPP = 12V, VPE = 14V, VHH = 12V, VCP = 5V, VCE = 5V, X = VIH or VIL)
MODE                                PINS
                                    CE          OE/VPP      A0          A9          VCC         OUTPUTS
# Read                              VIL         VIL         X           X           VCC         DOUT
Output Disable                      VIL         VIH         X           X           VCC         High Z
Standby (TTL)                       VIH         X           X           X           VCC         High Z
Standby (CMOS)                      VCC ±0.3V   X           X           X           VCC         High Z
# Program                           VIL         VPP         X           X           VCP         DIN
Program Verify                      VIL         VIL         X           X           VCC         DOUT
Program Inhibit                     VIH         VPP         X           X           VCP         High Z
# Erase                             VIL         VPE         VIL         VPE         VCE         DIH
Erase Verify                        VIL         VIL         X           X           3.75        DOUT
Erase Inhibit                       VIH         VPE         X           X           VCE         High Z
# Product Identifier-manufacturer   VIL         VIL         VIL         VHH         VCC         DA (Hex)
# Product Identifier-device         VIL         VIL         VIH         VHH         VCC         08 (Hex)