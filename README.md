
Compile the code using the provided makefile:

    make

To clear the directory (and remove .txt files):

    make clean

To run the router server on port 3000:

    python run.py -t router -p 3000

To run the master server on port 3010:

    python run.py -t master -p 3000 -ri <ROUTER IP> -rp <ROUTER PORT>

To run the client  

    python run.py -t client -i <ROUTER IP> -p <ROUTER PORT>
