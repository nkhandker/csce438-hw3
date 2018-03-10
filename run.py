#!/usr/bin/python

import os
import sys
import subprocess
from string import split 

def getopts(argv):
  opts = {}
  while argv: 
    if argv[0][0] == '-':
      opts[argv[0]] = argv[1]
    argv = argv[1:]
  return opts 

myargs = getopts(sys.argv)

exec_type = myargs['-t']
portno = myargs['-p']

if exec_type == 'master':
  ip_addr = subprocess.check_output(['hostname','-I']).rstrip("\n")
  if "-ri" in myargs: 
    rout_ip = myargs['-ri']
  else: 
    print("put in the routing ip using -ri <router ip address>")

  if "-rp" in myargs: 
    rout_port = myargs['-rp']
  else: 
    print("put in the routing ip using -ri <router ip address>")
 
  cmd = "./tsd -t master" + " -i " + ip_addr + " -p " + portno + " -r " + rout_ip + " -x " + rout_port 
  print cmd
  subprocess.call(split(cmd))

elif exec_type == 'router':  
  ip_addr = subprocess.check_output(['hostname','-I']).rstrip("\n")
  cmd = "./tsd -t router" + " -i " + ip_addr + " -p " + portno 
  print cmd
  subprocess.call(split(cmd))
elif exec_type == 'client': 
  if "-i" in myargs:  
    ip_addr = myargs['-i']
  else: 
    print("add ip like '-i <ip address>'")
  if "-u" in myargs: 
    usrname = myargs['-u']
  else: 
    print("input username like '-u <username>'")
  cmd = "./tsc -h " + ip_addr + " -p " + portno + " -u " + usrname
  print cmd
  subprocess.call(split(cmd))
else: 
  print(exec_type + "is not a valid type")








