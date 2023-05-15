import argparse
import serial


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

    s.write(bytearray([addr >> 24, addr >> 16, addr >> 8, addr >> 0]))

    if data is None:
        # Read command
        d = s.read(4)
        data = (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | (d[3] << 0)

    else:
        # Write command
        s.write(bytearray([data >> 24, data >> 16, data >> 8, data >> 0]))
        d = s.read(1)
        if d != 0x00:
            raise WishboneError("invalid ack byte received")
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

    byte = wishbone_serial(args.serial[0], args.address[0], args.value)
    if (byte) is not none:
        print(f'0x{byte:08x}')


if __name__ == "__main__":
    main()
