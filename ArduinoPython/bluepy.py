from bluepy import btle
import time

class MyDelegate(btle.DefaultDelegate):
    def __init__(self):
        btle.DefaultDelegate.__init__(self)

    def handleNotification(self, cHandle, data):
      decoded_data = data.decode('utf_8')
      print(decoded_data)


print("Connecting... 50:F1:4A:DA:C8:35")
p = btle.Peripheral("50:F1:4A:DA:C8:35")
print("Connected to Bluno")
p.setDelegate( MyDelegate() )
# Setup to turn notifications on, e.g.
svc = p.getServiceByUUID("0000dfb0-0000-1000-8000-00805f9b34fb")

while True:
    if p.waitForNotifications(1.0):
        print("Notified!")
        continue
