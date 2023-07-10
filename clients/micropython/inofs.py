import sys
import uselect
import struct
spoll = uselect.poll()
spoll.register(sys.stdin, uselect.POLLIN)

head = bytes("inofs", "ascii")
class inofs:
    @staticmethod
    def send(format: str, *args):
        data = ""
        if len(args) > 0:
            prec = format[0]*len(args) # 'f' single precision, 'd' double precision
            data = struct.pack("<"+prec, *args)
        msg = bytes(format, "ascii") + bytes(data, "ascii")
        length = struct.pack("<i", len(msg))
        sys.stdout.buffer.write(head + length + msg)
    @staticmethod
    def recv():
        if spoll.poll(0):
            # Check if the message header is correct
            remote_head = bytearray(sys.stdin.buffer.read(5), "little")
            if remote_head == head:
                # Check first 4 bytes for the data length
                length = int.from_bytes(sys.stdin.buffer.read(4), "little")
                # Check the next byte for double precision
                double_prec = int.from_bytes(sys.stdin.buffer.read(1), "little")
                # Compute how many values there are from previous info
                value_length = 8 if double_prec else 4
                values = int(length / value_length)
                # Read the data bytes
                buffer = bytearray(sys.stdin.buffer.read(length), "little")
                format = "<" + ("d"*values if double_prec else "f"*values)
                return struct.unpack(format, buffer)
            else:
                # Flush the buffer if the header is wrong
                while spoll.poll(0):
                    sys.stdin.buffer.read(1)
        return False
