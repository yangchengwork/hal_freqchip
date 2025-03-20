import os
import struct
import zlib


def fill_header(f_in_file, 
                    f_out_file,
                    version):
    code_length = os.path.getsize(f_in_file)
    f_out = open(f_out_file, 'wb')
    f_out.write(struct.pack('I', 0xAA5555AA))
    f_out.write(struct.pack('I', version))
    f_out.write(struct.pack('I', code_length))
    f_in = open(f_in_file, 'rb')
    array = f_in.read(code_length)
    image_crc = zlib.crc32(array)
    
    f_out.write(struct.pack('I', image_crc))
    f_out.seek(0x10)
    f_out.write(array)

def binary_to_c_array(input_file, output_file, array_name):
    with open(input_file, 'rb') as f:
        binary_data = f.read()

    with open(output_file, 'w') as f:
        f.write('#include <stdint.h>\n')
        f.write('const uint8_t {}[] = {{\n'.format(array_name))
        counter = 0
        for byte in binary_data:
            if counter == 0:
                f.write('    ')
            f.write('0x{:02X}, '.format(byte))
            counter = counter + 1
            if counter == 16:
                counter = 0
                f.write('\n')
        f.write('\n};')

fill_header("dsp_code_rom", "dsp_code_rom_burn.bin", 0x00010000)

binary_to_c_array("dsp_code_rom", "dsp_code_rom.c", "dsp_code_rom_buffer")
