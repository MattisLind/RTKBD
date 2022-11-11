# RTKBD
Emulation of a IBM PC RT keyboard.

## Dumps

The dumps directory contains vcd (Value Change Dumps) for the protocol used between the RT host and the RT keyboard.

The actual RESET sequence received after power on reset has not been grabbed since we got trouble when capturing this part. On the other hand the sequence initated with the RESET command is identical.

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

![](https://github.com/MattisLind/RTKBD/blob/main/images/CommandsReceivedF5hF3h4BhFCh58hFAh61h6Ah63h60hEDh00hF4h.png?raw=true)

After this the system boots normally

We can press the Caps Lock key which set the light.

![](https://github.com/MattisLind/RTKBD/blob/main/images/PressingCapsLockGettingEDhCommandWith00hArgumentBack.png?raw=true)

And the enter key generate MAKE (5Ah)

![](https://github.com/MattisLind/RTKBD/blob/main/images/Make5AhEnter.png?raw=true)

and BRAKE (F0h 5Ah)

![](https://github.com/MattisLind/RTKBD/blob/main/images/Break5AhEnter.png?raw=true)




