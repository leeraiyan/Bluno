import sys
import asyncio
import platform

from bleak import BleakClient


# you can change these to match your device or override them from the command line
CHARACTERISTIC_UUID = "0000dfb0-0000-1000-8000-00805f9b34fb"
ADDRESS = (
    "50:F1:4A:DA:C8:35"
    if platform.system() != "Darwin"
    else "B9EA5233-37EF-4DD6-87A8-2A875E821C46"
)


def notification_handler(sender, data):
    """Simple notification handler which prints the data received."""
    print("{0}: {1}".format(sender, data))


async def main(address, char_uuid):
    async with BleakClient(address) as client:
        print(f"Connected: {client.is_connected}")

        await client.start_notify(char_uuid, notification_handler)
        await asyncio.sleep(5.0)
        await client.stop_notify(char_uuid)


if __name__ == "__main__":
    asyncio.run(
        main(
            sys.argv[1] if len(sys.argv) > 1 else ADDRESS,
            sys.argv[2] if len(sys.argv) > 2 else CHARACTERISTIC_UUID,
        )
    )