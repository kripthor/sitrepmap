import datetime
import time
import threading
import sys
import os
from scapy.all import sniff
from geoip import geolite2
import SimpleHTTPServer
import SocketServer
import signal
import sys
import socket


PORT = 13690
seenIPS = dict()

def getMillis():
 return int(time.time())

def signal_handler(sig, frame):
 print('You pressed Ctrl+C!')
 os.kill(os.getpid(), 9)

def get_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # doesn't have to be reachable
        s.connect(('10.255.255.255', 1))
        IP = s.getsockname()[0]
    except:
        IP = '127.0.0.1'
    finally:
        s.close()
    print "My IP is "+IP
    return IP


OWNIP = get_ip()
# Custom Scapy Action function
def custom_action(packet):
 if (OWNIP == packet[0][1].dst):
  srcip = packet[0][1].src
  if (seenIPS.get(srcip) == None):
   gps = geolite2.lookup(srcip)
   if (gps != None):
    seenIPS[srcip] = [getMillis(), srcip, gps.country, str(gps.location)]
    print str(gps)
  else:
   seenIPS[srcip][0] = getMillis()
 return


def scapy_thread(name):
 # Setup sniff, filtering for IP traffic, limit packets b/c out of mem errors
 while True:
#  sniff(filter="ip", prn=custom_action, count=128)
  try:
   sniff(filter="not dst port "+str(PORT), prn=custom_action, count=128)
  except:
   print "Error while sniffing..."


def http_thread(name):
 Handler = SimpleHTTPServer.SimpleHTTPRequestHandler
 httpd = SocketServer.TCPServer(("", PORT), Handler)
 print "Serving at port", PORT
 httpd.serve_forever()




signal.signal(signal.SIGINT, signal_handler)
sthread = threading.Thread(target=scapy_thread, args=(1,))
sthread.start()
hthread = threading.Thread(target=http_thread, args=(2,))
hthread.start()

while True:
 millis = getMillis()
 f = open("events.txt", "w")
 f.write(str(millis)+'\n')

 for i in seenIPS.keys():
  #prune old
  if millis - seenIPS[i][0] > 700:
   del seenIPS[i]


 for key, value in sorted(seenIPS.items(), key=lambda item: item[1][0],reverse=True):
#  print("%s: %s" % (key, value))
  f.write(str(value)+'\n')

 f.close()
 time.sleep(1)
