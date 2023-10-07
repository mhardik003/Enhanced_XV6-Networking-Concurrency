# OSN Mini Project 2

## Instructions to fix the tester script for Spec 1 (getreadcount)

**Note:**
`test_2` won't be considered for evaluation since it involves handling concurrency. That is, marks won't be cut for the implementation of `getreadcount()` syscall even if `test_2` fails.

### 1. Ensure that gawk and expect are installed

- installing expect:

    ```bash
    sudo apt-get install expect
    ```

- installing gawk:

    ```bash
    sudo apt-get install gawk
    ```

### 2. Change the test_*.c files

- Go to the `./initial-xv6/tests/` directory.
- Replace the `test_1.c` and `test_2.c` files with the following code:

    [Link to test_1.c](https://iiitaphyd-my.sharepoint.com/:u:/g/personal/hitesh_goel_research_iiit_ac_in/EQOLB_Kxzt9JmMiRXrr1xcQBIMG7lZqFdk-ry8n7q5mAmA?e=5aJgDM)

    [Link to test_2.c](https://iiitaphyd-my.sharepoint.com/:u:/g/personal/hitesh_goel_research_iiit_ac_in/EWgoZHeNEAtPtQvwADx3tn8BGc6sN-4_pPcEeh-5I9VUaw?e=qZEBS1)

### 3. Add test files to user directory

- The same test files, i.e. `test_1.c` and `test_2.c` should be added to the `./initial-xv6/src/user/` directory.
- Consequently make changes to the Makefile as follows:

    The UPROGS part from around line 118 in `./initial-xv6/src/Makefile` will look as follows:

    ```c
    UPROGS=\
        $U/_cat\
        $U/_echo\
        $U/_forktest\
        $U/_grep\
        $U/_init\
        $U/_kill\
        $U/_ln\
        $U/_ls\
        $U/_mkdir\
        $U/_rm\
        $U/_sh\
        $U/_stressfs\
        $U/_usertests\
        $U/_grind\
        $U/_wc\
        $U/_zombie\
        $U/_schedulertest\
        $U/_test_1\
        $U/_test_2\
    ```

- You are free to add a user program for your getreadcount() system call in case you want to run it manually on xv6.

### 4. Change the executable

- Finally, we need to make a slight change to the `./tester/run-xv6-command.exp` file
- Change line 16 from

    `spawn make [lindex $argv 0] -f [lindex $argv 1] qemu-nox`
    
    to
    
    `spawn make [lindex $argv 0] -f [lindex $argv 1] qemu`

- Your final file will look like this:

    ```exp
    #! /usr/bin/env expect

    proc shutdown {} {
        # send command to halt qemu (ctrl-a x)
        # https://stackoverflow.com/questions/27050473/how-to-send-ctrl-a-then-d-in-expect
        send "\x01"; send "x"
        # make sure to wait for it all to stop
        # (without this, script was terminating before qemu quit -> bad)
        expect eof
    }

    # turn off timeout (perhaps make this flexible later)
    set timeout -1

    # build and launch xv6 on qemu
    spawn make [lindex $argv 0] -f [lindex $argv 1] qemu

    trap {
        shutdown
        exit 0
    } SIGINT

    # wait for initial prompt
    expect "init: starting sh\r"
    expect "$ "

    # send command as per command line
    send "[lindex $argv 2]\r"

    # wait for it to be done
    expect "$ "

    # shutdown qemu properly (avoid runaways)
    shutdown
    ```

