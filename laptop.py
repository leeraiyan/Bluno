from bluepy import btle
from datetime import datetime
from bluepy.btle import Scanner, DefaultDelegate
import logging
import time
import struct
import threading
import datetime

SERIAL_SERVICE = "0000dfb0-0000-1000-8000-00805f9b34fb"
# BLUNODICT = {"50:f1:4a:da:c8:35": {"id": 0, "name": "P1WristbandBluno", "discovered": 0, "acknowledged": 0, "buffer": b'', "bufferLength": 0, "dataComplete": 0}}

BLUNODICT = {"50:f1:4a:da:c8:35": {"id": 0, "name": "P1WristbandBluno", "discovered": 0, "acknowledged": 0,
                                   "buffer": b'', "bufferLength": 0, "dataComplete": 0, "transmittedPackets": 0},
             "30:e2:83:86:66:2b": {"id": 1, "name": "P1GunBluno", "discovered": 0, "acknowledged": 0,
                                   "buffer": b'', "bufferLength": 0, "dataComplete": 0, "transmittedPackets": 0},
             "30:e2:83:86:6a:98": {"id": 2, "name": "P1VestBluno", "discovered": 0, "acknowledged": 0,
                                   "buffer": b'',"bufferLength": 0, "dataComplete": 0,  "transmittedPackets": 0}}

# BLUNODICT = {"30:e2:83:86:66:2b": {"id": 1, "name": "P1GunBluno", "discovered": 0, "acknowledged": 0, "buffer": b'', "bufferLength": 0}}

BLUNO_ID_to_ADDR = {0: "50:f1:4a:da:c8:35", 1: "30:e2:83:86:66:2b", 2:"30:e2:83:86:6a:98"}
THIS_MACHINE_MAC = "70:CF:49:4F:51:7C"
NO_OF_BLUNOS = 3
resetFlag = 0

#this flag sends a reset command to the Blunos when this client program first boots
BOOTRESETFLAGS = [0,0,0]

class ConnectionHandlerThread (threading.Thread):
    def __init__(self, peri):
        threading.Thread.__init__(self)
        self.peri = peri

    def run(self):
        try:
            while (True):
                if self.peri.waitForNotifications(1.0):
                    continue
        except Exception as e:
            logging.error("The thread has failed")



class Bluno:
    def __init__(self, address, id, name, discovered, handshaken = 0, acknowledged = 0):
        self.id = id
        self.address = address
        self.name = name
        self.discovered = discovered
        self.handshaken = 0
        self.acknowledged = 0

    def resetBluno(self):
        self.discovered = 0
        self.handshaken = 0
        self.acknowledged = 0

    def resetHandshake(self):
        self.handshaken = 0
        BLUNODICT[self.address]["acknowledged"] = 0

    def printStatus(self):
        print(" ID", self.id)
        print(" name", self.name)
        print(" address", self.address)
        print(" discovered?", self.discovered)
        print(" handshake?", self.handshaken)
        print(" ~~~~~~~")



class MyDelegate(btle.DefaultDelegate):
    def __init__(self, bluno, serialCharacteristic):
        btle.DefaultDelegate.__init__(self)
        self.resetCounter = 0
        self.bluno = bluno
        self.serialCharacteristic = serialCharacteristic

    def handleNotification(self, cHandle, data):
        if BLUNODICT[self.bluno.address]["acknowledged"] == 0:
            try:
                decoded_data = data.decode('utf_8')
                if decoded_data == "A":
                    print("Received ACK from Bluno")
                    BLUNODICT[self.bluno.address]["acknowledged"] = 1

                else:
                    print("Data from Bluno:", data)
            except Exception as e:
                logging.error("The handshake reply is not normal, reset the Bluno")
                self.serialCharacteristic.write("R".encode("utf-8"), withResponse=False)

        #this is how we handle data from the wristbandBluno
        elif data != b'A' and BLUNODICT[self.bluno.address]["id"] == 0:
            try:
                BLUNODICT[self.bluno.address]["buffer"] += data
                BLUNODICT[self.bluno.address]["bufferLength"] = len(BLUNODICT[self.bluno.address]["buffer"])

                if BLUNODICT[self.bluno.address]["bufferLength"] == 12:
                    packet = struct.unpack('<hhhhhh', BLUNODICT[self.bluno.address]["buffer"])
                    print(BLUNODICT[self.bluno.address]["name"], "says acc_x, acc_y, acc_z, yaw, pitch, roll:",
                          packet[0], packet[1], packet[2], packet[3], packet[4], packet[5])

                    BLUNODICT[self.bluno.address]["buffer"] = b''
                    BLUNODICT[self.bluno.address]["bufferLength"] = 0
                    BLUNODICT[self.bluno.address]["dataComplete"] = 1
                    BLUNODICT[self.bluno.address]["transmittedPackets"] += 1
                    if BLUNODICT[self.bluno.address]["transmittedPackets"] % 100:
                        self.serialCharacteristic.write("G".encode("utf-8"), withResponse=False)
                    # print("transmitted Packets:", BLUNODICT[self.bluno.address]["transmittedPackets"])

                elif BLUNODICT[self.bluno.address]["bufferLength"] > 12:
                    dataToUnpack = BLUNODICT[self.bluno.address]["buffer"][0:12]
                    BLUNODICT[self.bluno.address]["buffer"] = BLUNODICT[self.bluno.address]["buffer"][12:]
                    BLUNODICT[self.bluno.address]["bufferLength"] = len(BLUNODICT[self.bluno.address]["buffer"])

                    packet = struct.unpack('<hhhhhh', dataToUnpack)
                    # print("Trying to recreate data:", int.from_bytes(data, byteorder='little', signed=True))
                    print(BLUNODICT[self.bluno.address]["name"], "says acc_x, acc_y, acc_z, yaw, pitch, roll:",
                          packet[0], packet[1], packet[2], packet[3], packet[4], packet[5])


                    BLUNODICT[self.bluno.address]["dataComplete"] = 1
                    BLUNODICT[self.bluno.address]["transmittedPackets"] += 1
                    if BLUNODICT[self.bluno.address]["transmittedPackets"] % 100:
                        self.serialCharacteristic.write("G".encode("utf-8"), withResponse=False)
                    # print("transmitted Packets:", BLUNODICT[self.bluno.address]["transmittedPackets"])

            except Exception as e:
                logging.error("The data fragments are weird")
                print(data)

        # this is how we handle data from the gunBluno or the vestBluno
        elif data != b'A' and BLUNODICT[self.bluno.address]["id"] == 1:
            if data == b'\xff':
                print(BLUNODICT[self.bluno.address]["name"], "says Gunshot!")
            BLUNODICT[self.bluno.address]["dataComplete"] = 1
            BLUNODICT[self.bluno.address]["transmittedPackets"] += 1
            if BLUNODICT[self.bluno.address]["transmittedPackets"] % 100:
                self.serialCharacteristic.write("G".encode("utf-8"), withResponse=False)

        elif data != b'A' and BLUNODICT[self.bluno.address]["id"] == 2:
            if data == b'\xff':
                print(BLUNODICT[self.bluno.address]["name"], "says I've been hit!")
            BLUNODICT[self.bluno.address]["dataComplete"] = 1
            BLUNODICT[self.bluno.address]["transmittedPackets"] += 1
            if BLUNODICT[self.bluno.address]["transmittedPackets"] % 100:
                self.serialCharacteristic.write("G".encode("utf-8"), withResponse=False)

class ScanDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)

    def handleDiscovery(self, dev, isNewDev, isNewData):
        if str(dev.addr) in BLUNODICT:
            BLUNODICT[dev.addr]["discovered"] = 1
            print(dev.addr, "(", BLUNODICT[dev.addr]["name"] ,") has been discovered")




def checkBlunoAdvertisement():
    scanner = Scanner().withDelegate(ScanDelegate())
    devices = scanner.scan(3.0)

    blunoList = []
    for dev in devices:
        if str(dev.addr) in BLUNODICT and BLUNODICT[dev.addr]["discovered"] == 1:
            p1wbBluno = Bluno(str(dev.addr), BLUNODICT[dev.addr]["id"], BLUNODICT[dev.addr]["name"], discovered = 1)
            blunoList.append(p1wbBluno)
    return blunoList


def handshakeBluno(bluno, peri, serialCharacteristic):
    try:
        # global BOOTRESETFLAG[BLUNODICT[bluno]["id"]]
        if not BOOTRESETFLAGS[BLUNODICT[bluno.address]["id"]]:
            BOOTRESETFLAGS[BLUNODICT[bluno.address]["id"]] = 1
            print("Client has just booted, so reset the Bluno")
            serialCharacteristic.write("R".encode("utf-8"), withResponse=False)
        else:
            serialCharacteristic.write("H".encode("utf-8"), withResponse = False)

        if peri.waitForNotifications(1) and BLUNODICT[bluno.address]["acknowledged"]:
            serialCharacteristic.write("A".encode("utf-8"), withResponse=False)
            bluno.handshaken = 1

        return bluno

    except Exception as e:
        logging.error("There was a problem with the handshake", e)
        return bluno



def connectToBluno(bluno, backoff = 0):
    try:
        peri = btle.Peripheral(bluno.address)
        print("Connected to ", bluno.name , "(", peri.addr ,")")
        serialService = peri.getServiceByUUID(SERIAL_SERVICE)
        serialCharacteristic = serialService.getCharacteristics()[0]
        peri.setDelegate(MyDelegate(bluno, serialCharacteristic))


        handshakeCounter = 0
        #lets try the handshake
        while(bluno.handshaken == 0):
            print("Attempting handshake", str(handshakeCounter+1))
            bluno = handshakeBluno(bluno, peri, serialCharacteristic)
            handshakeCounter += 1

            if handshakeCounter >= 3:
                peri.disconnect()
                raise Exception("Tried 3 handshakes")
        print("Successful Handshake with", bluno.address)

        return peri, serialCharacteristic, backoff

    except Exception as e:
        backoff = min(5, backoff+1)
        logging.error("Theres a problem connecting to Bluno")
        print("attempting to reconnect to ", bluno.address, "in ", backoff, "seconds")



        bluno.handshaken = 0
        BLUNODICT[bluno.address]["acknowledged"] = 0
        # global BOOTRESETFLAG
        # BOOTRESETFLAG = 0

        BLUNODICT[bluno.address]["buffer"] = b''
        BLUNODICT[bluno.address]["bufferLength"] = 0
        if peri is not None:
            peri.disconnect()
        #simple linear backoff
        time.sleep(backoff)
        #then reconnect
        # connectToBluno(bluno, backoff)
        return peri, serialCharacteristic, backoff

def beginCommunication(blunoList):
    try:
        connections = []
        serialCharacteristics = []
        for bluno in blunoList:
            peri, serialCharacteristic, backoff = connectToBluno(bluno)
            serialCharacteristics.append(serialCharacteristic)
            connections.append(peri)
        if len(connections) != len(blunoList):
            raise Exception("Not enough handshakes to continue")
        begin_time = datetime.datetime.now()
        print("Begin Getting Data")
        #we do like a round robin for the connections, we write a D to the bluno and wait for a complete packet to return
        while(True):
            for i in range(len(blunoList)):
                # print("writing to", serialCharacteristics[i].peripheral.addr)
                # serialCharacteristics[i].write("D".encode("utf-8"), withResponse=False)
                if connections[i].waitForNotifications(1):
                    continue

                # while BLUNODICT[blunoList[i].address]["dataComplete"] != 1:
                #     if connections[i].waitForNotifications(0):
                #         continue
                BLUNODICT[blunoList[i].address]["dataComplete"] = 0

            # for testing throughput
            # if BLUNODICT["50:f1:4a:da:c8:35"]["transmittedPackets"] >= 1000:
            #     print(datetime.datetime.now() - begin_time)
            #     break



    except Exception as e:
        logging.error("Communication Interrupted")
        print("Restarting ALL communications")
        # for serialCharacteristic in serialCharacteristics:
        #     if serialCharacteristic is not None:
        #         serialCharacteristic.write("R".encode("utf-8"), withResponse=False)

        # for i in range(len(connections)):
        #     if serialCharacteristics[i] is not None:
        #         serialCharacteristics[i].write("R".encode("utf-8"), withResponse=False)
        #     if connections[i] is not None:
        #         connections[i].disconnect()
        for connection in connections:
            if connection is not None:
                connection.disconnect()

        for bluno in blunoList:
            BLUNODICT[bluno.address]["dataComplete"] = 0
            bluno.handshaken = 0
        blunoList = []
        while len(blunoList) != NO_OF_BLUNOS:
            blunoList = checkBlunoAdvertisement()

        # for connection in connections:
        #     if connection is not None:
        #         connection.disconnect
        beginCommunication(blunoList)


print("Client Side Starting, scanning for Blunos", list(BLUNODICT.keys()))
#Ensure all devices are advertising before moving on to handshake
#Create list of Bluno objects
blunoList = []
while len(blunoList) != NO_OF_BLUNOS:
    blunoList = checkBlunoAdvertisement()


#if you need to see which bluno objects were created
# print("Blunos:")
# for bluno in blunoList:
#     bluno.printStatus()

#Establish Connection with Bluno
beginCommunication(blunoList)
