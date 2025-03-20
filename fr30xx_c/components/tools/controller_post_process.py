import os, sys
import struct
import zlib


def fill_header(f_in_file, 
                    f_out_file, 
                    version):
    code_length = os.path.getsize(f_in_file)
    f_out = open(f_out_file, 'wb')
    f_out.write(struct.pack('I', 0x55AAAA55))
    f_out.write(struct.pack('I', version))
    f_out.write(struct.pack('I', code_length))
    f_in = open(f_in_file, 'rb')
    array = f_in.read(code_length)
    image_crc = zlib.crc32(array)    
    f_out.write(struct.pack('I', image_crc))
    f_out.seek(0x10)
    f_out.write(array)

fill_header("controller_code", "controller_code_burn.bin", 0x00010000)
