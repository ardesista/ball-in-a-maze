#!/usr/bin/env python3

import os, sys, re, argparse
import numpy as np
import cv2

def im16_to_im24(im16):
    return np.dstack(
        (((im16 & 0x1f) * 255 / 31).round().astype(np.uint8),
        (((im16 >> 5) & 0x3f) * 255 / 63).round().astype(np.uint8),
        ((im16 >> 11) * 255 / 31).round().astype(np.uint8)))

def im24_to_im16(im24):
    t = im24 / 255.0
    return (
        (t[:, :, 0] * 31).round().astype(np.uint16) |\
        ((t[:, :, 1] * 63).round().astype(np.uint16) << 5) |\
        ((t[:, :, 2] * 31).round().astype(np.uint16) << 11))

def im16_write(path, im, im_id=None):
    if im_id is None:
        im_id, _ = os.path.splitext(os.path.basename(path))

    with open(path, 'wt') as f:
        f.write('#ifndef {}_WIDTH\n#define {}_WIDTH  {}\n#define {}_HEIGHT {}\n'.format(im_id, im_id, im.shape[1], im_id, im.shape[0]))
        f.write('const unsigned short {}[] = {{ '.format(im_id))
        f.write(', '.join([ '0x{:04x}'.format(b) for b in im.flatten() ]))
        f.write(' }};\n#endif // {}_WIDTH\n'.format(im_id))

def im16_read(path):
    with open(path, 'rt') as f:
        h1, h2, h3, dt, _ = [ x for x in f.readlines() if len(x) > 0 ]

        matches = re.match(r'^#ifndef\s+([a-zA-Z_][a-zA-Z0-9_]*)_WIDTH\s*$', h1)
        assert matches is not None
        im_id = matches.group(1)

        matches = re.match(r'^#define\s+([a-zA-Z_][a-zA-Z0-9_]*)_WIDTH\s+([0-9]+)\s*$', h2)
        assert matches is not None
        im_id2, width_str = matches.groups()
        assert im_id == im_id2
        width = int(width_str)

        matches = re.match(r'^#define\s+([a-zA-Z_][a-zA-Z0-9_]*)_HEIGHT\s+([0-9]+)\s*$', h3)
        assert matches is not None
        im_id3, height_str = matches.groups()
        assert im_id == im_id3
        height = int(height_str)

        matches = re.match(r'^const\s+unsigned\s+short\s+([a-zA-Z_][a-zA-Z0-9_]*)\[\s*\]\s*=\s*\{([0-9a-fA-Fx, ]*)\};\s*$', dt)
        assert matches is not None
        im_id4, data_str = matches.groups()
        assert im_id == im_id4
        data = [ int(x, 16) for x in data_str.split(',') if len(x) > 0 ]
        assert len(data) == (width * height * 2)

        return im_id, np.ndarray((height, width), dtype=np.uint16, buffer=bytes(data))

if __name__ == '__main__':
    parser = argparse.ArgumentParser('Convert between images and 16-bit (565) C headers')
    parser.add_argument('-r', '--header-to-image', action='store_true', default=False, help='Parse header and output image file (default: false)')
    parser.add_argument('-o', '--output', type=str, required=True, help='Output path')
    parser.add_argument('path', type=str, help='Input path')

    args = parser.parse_args()

    if args.header_to_image:
        im_id, im = im16_read(args.path)
        cv2.imwrite(args.output, im16_to_im24(im))
    else:
        im = cv2.imread(args.path)
        im16_write(args.output, im24_to_im16(im))

