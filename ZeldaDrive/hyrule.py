#! /usr/bin/python3
import os

PATH_NAME = 256

def int32_to_array(val):
    arr = bytearray(4)
    arr[0] = (val >> 0) & 0xff
    arr[1] = (val >> 8) & 0xff
    arr[2] = (val >> 16) & 0xff
    arr[3] = (val >> 24) & 0xff
    return arr

def string_to_array(arr, _str):
    _arr = _str.encode()
    _idx = 0
    for _byte in _arr:
        arr[_idx] = _byte
        _idx = _idx + 1


def write_drive_file(_file_dst, _path):
    name = bytearray(PATH_NAME)
    size = os.path.getsize(_path)
    string_to_array(name, _path)
    _file_dst.write(name)
    _file_dst.write(int32_to_array(size))
    print('[File] Found file:%s size:%s' % (_path, size))
    with open(_path, 'rb') as f:
        while True:
            context = f.read(512)
            if context == ''.encode():
                break
            _file_dst.write(context)

def enumerate_directory(_file_dst, _dir,):
    nodes = os.listdir(_dir)
    for _node in nodes:
        _path = _dir + '/' + _node
        if os.path.isdir(_path):
            enumerate_directory(_file_dst, _path)
        else:
            write_drive_file(_file_dst, _path)

if __name__ == '__main__':
    os.chdir('./root')
    with open('../../Zelda.drive', 'wb') as f:
        enumerate_directory(f, '.')
