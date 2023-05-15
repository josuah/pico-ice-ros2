import argparse
import serial
import struct


class WishboneError(Exception):
    pass


def wishbone_serial(port, addr, data):
    s = serial.Serial(port)

    if data is None:
        # Read command
        s.write(b'\x01')
    else:
        # Write command
        s.write(b'\x00')

    # TODO: support buffer up to 0x55 bytes
    s.write(b'\x04')
    s.write(struct.pack('>I', addr))

    if data is None:
        # Read command
        data = struct.unpack('>I', s.read(4))[0]

    else:
        # Write command
        s.write(struct.pack('>I', data))
        d = s.read(1)
        if d[0] != 0x00:
            raise WishboneError(f"invalid ack byte received: 0x{d[0]:02x}")
        data = None

    s.close()
    return data


def main():
    parser = argparse.ArgumentParser(
        description='access a wishbone bridge over a serial protocol')
    parser.add_argument('--serial', dest='serial', nargs=1, required=True,
        help='serial port file to use for the communication')
    parser.add_argument('address', nargs=1, type=lambda x : int(x, 0),
        help='addres of the register to query')
    parser.add_argument('value', nargs='?', type=lambda x : int(x, 0),
        help='value to write, or if absent, a read operation is done')
    args = parser.parse_args()

    data = wishbone_serial(args.serial[0], args.address[0], args.value)
    if data is not None:
        print(f'0x{data:08x}')


if __name__ == "__main__":
    main()
