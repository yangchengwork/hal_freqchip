import os
import struct
import zlib


def fill_header(f_in_file, 
                    f_out_file):
    code_length = os.path.getsize(f_in_file)
    f_out_create = open(f_out_file,'wb');
    f_out_create.close()
    f_out = open(f_out_file, 'r+b')
    f_in = open(f_in_file, 'rb')
    array = f_in.read(code_length)
    f_out.write(array)
    f_out.seek(0x150)
    f_out.write(struct.pack('I', code_length))
    f_out.seek(0)
    arrayout = f_out.read(code_length)
    image_crc = zlib.crc32(arrayout)
    f_out.seek(code_length)
    f_out.write(struct.pack('I',image_crc))

fill_header("Project.bin", "Project_burn.bin")
