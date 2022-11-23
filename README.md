# RTKBD
This project aims at emulating a IBM PC RT keyboad with a standard IBM model M keyboard. There are many similarities but still some differences. The major difference is that a standard PS/2 model M is using scan code set 2 while a IBM PC RT keyboard (and most terminal keyboards) are using scan code set 3.
The IBM RT PC keyboard also provide the ID code as part of the RESET sequence. This is not the case with a standard IBM Model M keyboard.

Since I had trouble getting emulation working with the existing PS2Dev Arduino library I took the code out of the existing rubber dome keyboard that someone already had dumped and disassmbled it.

## Dumps

To get more information I cobbled together some code in a STM32 that could sniff the data and clock lines of the PS/2 bus. It monitored any changes on these lines and stored a timestamp (in microseconds relative to the start of the microcontroller).

The dumps directory contains vcd (Value Change Dumps) for the protocol used between the RT host and the RT keyboard.

The actual RESET sequence received after power on reset has not been grabbed since I got trouble when capturing this part. On the other hand the sequence initated with the RESET command is identical.

This sequence below happens directly after start of the computer until it goes into startup step 09.

![Reset flow 333 ms from RESET command to actual RESET taking place](https://github.com/MattisLind/RTKBD/blob/main/images/333msFromResetCmdToResponse.png?raw=true)

FFh (RESET) command followed by the FAh (ACK) response.

![FFh (RESET) command followed by the FAh (ACK) response](https://github.com/MattisLind/RTKBD/blob/main/images/ResetCommandFollowedByACK.png?raw=true)


The first event is the FFh command (RESET) sent by the host and the response FAh (ACK) from the keyboard. Then there is a 333 ms delay from the RESET command until it responds with the AAh (OK test response). Then the keyboard also emits the ID. In this case BFh B1h.

![AAh response and then ID codes with 2ms delay](https://github.com/MattisLind/RTKBD/blob/main/images/ResetResponseAAhBFhB1h_2msBetweenSuccessiveTransmissions.png?raw=true)

This is the AAh reponse code and the with two ms delay the ID code, BFh and B1h.

The F5h and F4h commands received and ACKed by the keyboard

![The F5h and F4h commands received and ACKed by the keyboard](https://github.com/MattisLind/RTKBD/blob/main/images/F5hCommandToF4Command15ms.png?raw=true)

The F5h command and the ACK.

![The F5h command and the ACK](https://github.com/MattisLind/RTKBD/blob/main/images/F5hCommandReceivedAckSent.png?raw=true)


The following events takes place just after the last startup step, before it shows a cursor on screen and boots AIX.

It performs two consecutive RESET operations and then send the E6h command. 

![](https://github.com/MattisLind/RTKBD/blob/main/images/TwoConsecutiveResetOperationsAndThenAnE6hCommandWhichIsSentAnFChInResponse.png?raw=true)

The keyboard will respond with FCh to this command since it is not supported.

![](https://github.com/MattisLind/RTKBD/blob/main/images/E6hCommandReceivedAndFChSentInResponse.png?raw=true)

After this the host sends 

|  Code   |     Description |
|---------|-----------------|
|  F5     |  Disable Keyboard    |
| F3 4B   |  Set typematic  |
| FC 58   |  Action key     |
| FA 61 6A 63 60 |  Arrow keys |
| ED 00   |  Set LEDs       |
|  F4     |  Enable Keybaord |

The keyboard is thus setup for Make/Break/Typematic for all keys except for the action key (58h) which is Make/Break only 

![](https://github.com/MattisLind/RTKBD/blob/main/images/CommandsReceivedF5hF3h4BhFCh58hFAh61h6Ah63h60hEDh00hF4h.png?raw=true)

After this the system boots normally

We can press the Caps Lock key which set the light.

![](https://github.com/MattisLind/RTKBD/blob/main/images/PressingCapsLockGettingEDhCommandWith00hArgumentBack.png?raw=true)

And the enter key generate MAKE (5Ah)

![](https://github.com/MattisLind/RTKBD/blob/main/images/Make5AhEnter.png?raw=true)

and BRAKE (F0h 5Ah)

![](https://github.com/MattisLind/RTKBD/blob/main/images/Break5AhEnter.png?raw=true)


## Implementation

There are two parts, one that communicate with the host, emulating a RT keyboard. This is basically the reverseengineered code from the disassembly of the original keyboard. It is implemented as faithful as possible trying to mimic all the delays that were used in the original implementation. In the pursuit to be as exact as possible many of the jump instruction used in the original code has been implemented as goto in C. Perhaps not the moste beautiful code but it is very close to the original.

The second part is an interrupt driven host-emulator which interfaces with the IBM PS/2 keyboard. It tries to reset the keyboard at start and then waits for the reset reseponse. Whenever this comes it will try to execute a set of orders to the keyboard to force it into scan code set 3 and also set all keys (except action which is just make/brake) as make/break/typematic.

After this the device will monitor for incoming messages and forward it to the other end.

