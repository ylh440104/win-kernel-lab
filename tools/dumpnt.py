import struct, sys

path = sys.argv[1]
targets = [(0x1406b68ab, 0x120, 'KiSystemServiceExitPico'),
           (0x1406b6230, 0x60, 'KiSystemCall64_area'),
           (0x140a89878, 0x40, 'PsPicoSystemCallDispatch')]

ImageBase = 0x140000000
data = open(path, 'rb').read()

assert data[:2] == b'MZ'
peoff = struct.unpack_from('<I', data, 0x3c)[0]
assert data[peoff:peoff+4] == b'PE\0\0'
coff = peoff + 4
nsect = struct.unpack_from('<H', data, coff + 2)[0]
optsize = struct.unpack_from('<H', data, coff + 16)[0]
opt = coff + 20
magic = struct.unpack_from('<H', data, opt)[0]
print('magic %x nsect %d optsize %d' % (magic, nsect, optsize))
secs = opt + optsize
sections = []
for i in range(nsect):
    off = secs + i * 40
    name = data[off:off+8].rstrip(b'\0').decode(errors='replace')
    vsize, vaddr, rawsize, rawptr = struct.unpack_from('<IIII', data, off + 8)
    sections.append((name, vaddr, vsize, rawptr, rawsize))
    print('  %-8s va=%08x vsize=%08x raw=%08x rsize=%08x' % (name, vaddr, vsize, rawptr, rawsize))

def rva_to_off(rva):
    for name, vaddr, vsize, rawptr, rawsize in sections:
        if vaddr <= rva < vaddr + max(vsize, rawsize):
            return rawptr + (rva - vaddr)
    return None

for addr, length, label in targets:
    rva = addr - ImageBase
    off = rva_to_off(rva)
    print('\n===== %s  RVA=%08x  file_off=%s =====' % (label, rva, hex(off) if off else 'NOT MAPPED'))
    if off is None:
        continue
    chunk = data[off:off+length]
    for i in range(0, len(chunk), 16):
        row = chunk[i:i+16]
        hexs = ' '.join('%02x' % b for b in row)
        asc = ''.join(chr(b) if 32 <= b < 127 else '.' for b in row)
        print('%08x  %-47s  %s' % (rva + i, hexs, asc))
