

import asyncio
import platform
import sys

from bleak import BleakClient, BleakScanner
from bleak.exc import BleakError


ADDRESS = (
    "50:F1:4A:DA:C8:35"
    if platform.system() != "Darwin"
    else "B9EA5233-37EF-4DD6-87A8-2A875E821C46"
)


async def main(ble_address: str):
    device = await BleakScanner.find_device_by_address(ble_address, timeout=20.0)
    if not device:
        raise BleakError(f"A device with address {ble_address} could not be found.")
    async with BleakClient(device) as client:
        svcs = await client.get_services()
        print("Services:")
        for service in svcs:
            print("The service", service, "has the following characteristics:")
            characteristics = service.characteristics
            for characteristic in characteristics:
                print(characteristic)
                value = await client.read_gatt_char(characteristic)
                print("Pre-Write Value: {0}".format(value))


if __name__ == "__main__":
    asyncio.run(main(sys.argv[1] if len(sys.argv) == 2 else ADDRESS))

