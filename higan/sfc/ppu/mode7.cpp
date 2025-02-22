auto PPU::Background::clip(int n) -> int {
  //13-bit sign extend: --s---nnnnnnnnnn -> ssssssnnnnnnnnnn
  return n & 0x2000 ? (n | ~1023) : (n & 1023);
}

//H = 28
auto PPU::Background::beginMode7() -> void {
  latch.hoffset = ppu.io.hoffsetMode7;
  latch.voffset = ppu.io.voffsetMode7;
}

auto PPU::Background::runMode7() -> void {
  int a = (int16)ppu.io.m7a;
  int b = (int16)ppu.io.m7b;
  int c = (int16)ppu.io.m7c;
  int d = (int16)ppu.io.m7d;

  int hcenter = (int13)ppu.io.m7x;
  int vcenter = (int13)ppu.io.m7y;
  int hoffset = (int13)latch.hoffset;
  int voffset = (int13)latch.voffset;

  uint x = mosaic.hoffset;
  uint y = ppu.bg1.mosaic.voffset;  //BG2 vertical mosaic uses BG1 mosaic size

  if(--mosaic.hcounter == 0) {
    mosaic.hcounter = mosaic.size + 1;
    mosaic.hoffset += mosaic.size + 1;
  }

  if(ppu.io.hflipMode7) x = 255 - x;
  if(ppu.io.vflipMode7) y = 255 - y;

  int originX = (a * clip(hoffset - hcenter) & ~63) + (b * clip(voffset - vcenter) & ~63) + (b * y & ~63) + (hcenter << 8);
  int originY = (c * clip(hoffset - hcenter) & ~63) + (d * clip(voffset - vcenter) & ~63) + (d * y & ~63) + (vcenter << 8);

  int pixelX = originX + a * x >> 8;
  int pixelY = originY + c * x >> 8;
  uint16 paletteAddress = (uint3)pixelY << 3 | (uint3)pixelX;

  uint7 tileX = pixelX >> 3;
  uint7 tileY = pixelY >> 3;
  uint16 tileAddress = tileY << 7 | tileX;

  bool outOfBounds = (pixelX | pixelY) & ~1023;

  uint8 tile = ppu.io.repeatMode7 == 3 && outOfBounds ? 0 : ppu.vram[tileAddress] >> 0;
  uint8 palette = ppu.io.repeatMode7 == 2 && outOfBounds ? 0 : ppu.vram[tile << 6 | paletteAddress] >> 8;

  uint priority;
  if(id == ID::BG1) {
    priority = io.priority[0];
  } else if(id == ID::BG2) {
    priority = io.priority[palette.bit(7)];
    palette.bit(7) = 0;
  }

  if(palette == 0) return;

  if(io.aboveEnable) {
    output.above.priority = priority;
    output.above.palette = palette;
    output.above.paletteGroup = 0;
  }

  if(io.belowEnable) {
    output.below.priority = priority;
    output.below.palette = palette;
    output.below.paletteGroup = 0;
  }
}
