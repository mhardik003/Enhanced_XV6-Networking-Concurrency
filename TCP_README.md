# Implementation of TCP through UDP Sockets

## Difference Between Standard TCP and implemented TCP


1. We aren't doing a three way handshake which is there in standard TCP. 

2. The communication is one sided meaning only one will receive at particular instant and other will send it (both sending and receive is not concurrent for one server).

2. We aren't using a SYN bit (sequence number for first packet) instead we send the number of packets to be sent and make use of it to check connection.

3. Not using sliding window approach for flow control

4. The packet size is fixed in our program, which means that if the string has 1024 bytes then except the last one each packet will have maximum size (i.e. if max size is 100 then there will be 10 packets of size 100 and last packet of size 24), unlike standard TCP in which size can vary.

5. Other flags like FYN and RST used for termination aren't being used.

6. We are sending the acknoledgement bit for each package, unlike TCP

<br>

---

## Handling  Flow Control

Flow control in TCP regulates data transmission to match the receiver's processing capacity, preventing overload and ensuring efficient communication.

For example, if a PC sends data to a smartphone that is slowly processing received data, the smartphone must be able to regulate the data flow so as not to be overwhelmed.

One of the ways to do this is to use a sliding window, which can be used in our code to implement flow control.

<br>

### Sliding Window
TCP uses a sliding window mechanism to implement flow control. The sender and receiver each maintain a window of acceptable sequence numbers. The sender can only transmit data within the sender's window, and the receiver can only accept data within the receiver's window.

<br>

Adding these steps to our code will help us implement flow control:
    
    - Sender maintains a window of packets allowed to be in transit.
    - When an ACK is received, the window slides, and the next set of packets (if available) is sent.
    - Receiver’s acknowledgment allows it to manage its buffer and prevent overflow.
    - Receiver dynamically adjusts its window size based on its buffer availability and sends this in ACKs.
    - Sender adjusts its window size based on ACKs and possible network congestion using algorithms (like slow start and congestion avoidance )
    