
projects to practice
1. chat app between two people
2. chat app between 3+ people ( a chat room)
3. multiple chat rooms containing 2 people
4. multiple chat rooms containing 3+ people
5. blackjack btwn server and client
6. blackjack btween 3+ people
7. make a user interface for everything via javascript#


___CHAT ROOM PLAN___
Musts:
- must be able to let users type while also receive and display information concurrently
    - plan to use threads
    - one thread for asking for user input
    - second thread fo receiving input from server and displaying to terminal
- all data will be sent to host/server and server will multicast to all clients connected
    - will use multicasting capabilities of TCP
    - will potentially move to SCTP for its multicasting capabilities
Resources:
- Unix Network Programming Volume 1, Third Edition
    - Chapter 21: Multicasting
    - Chapter 26: Threads
- Advaned Programming in the Unix Environment, Third Edition
    - Chapter 11: Threads
    - Chapter 12: Thread Control