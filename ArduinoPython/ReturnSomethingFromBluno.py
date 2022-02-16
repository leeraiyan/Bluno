import asyncio
from bleak import BleakClient

address = "50:F1:4A:DA:C8:35"
MODEL_NBR_UUID = "00002a24-0000-1000-8000-00805f9b34fb"

async def connect(address, loop):
    async with BleakClient(address) as client:
        model_number = await client.read_gatt_char(MODEL_NBR_UUID)
        print("Model Number: {0}".format("".join(map(chr, model_number))))

loop = asyncio.get_event_loop()
loop.run_until_complete(connect(address, loop))