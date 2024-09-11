#!/usr/bin/env python3

import cv2
import numpy as np

FONT_SIZE = 12 # px

if __name__ == '__main__':
    im = cv2.imread('/tmp/12x12.png', 0)
    rows = im.shape[0] // FONT_SIZE
    cols = im.shape[1] // FONT_SIZE

    chars_lines = []
    for i in range(rows):
        for j in range(cols):
            c = im[(i * FONT_SIZE):((i + 1) * FONT_SIZE), (j * FONT_SIZE):((j + 1) * FONT_SIZE)].flatten()
            bs = []
            b = 0
            for ci, ck in enumerate(c):
                bi = ci % 8
                if ck > 127:
                    b |= (0x1 << bi)
                if bi == 7:
                    bs.append('0x{:02x}'.format(b))
                    b = 0
            chars_lines.append('{{ {} }}'.format(', '.join(bs)))
    print(',\n'.join(chars_lines))
