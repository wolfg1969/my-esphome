import serial
s = serial.Serial('/dev/ttyUSB0', 19200, timeout=0.5)
s.reset_input_buffer()
s.write(bytes([0x35, 0x16]))
d = s.read(4)
print(d.hex(' ') or 'no response')
if len(d) == 4:
    ok = (((0x16 + d[0] + d[1] + d[2]) & 0xFF) ^ 0xFF) == d[3]
    print('checksum ok' if ok else 'checksum bad')
