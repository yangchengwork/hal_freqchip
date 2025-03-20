import sys

if __name__ == '__main__':
    '''
    argv[1]: project name
    argv[2]: output path
    '''

    print("%s\\%s.bin" % (sys.argv[2], sys.argv[1]))
