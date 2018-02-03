//==================================================================================================================================
//  Simple Kernel: Kernel Entrypoint
//==================================================================================================================================
//
// Version 0.x
//
// Author:
//  KNNSpeed
//
// Source Code:
//  https://github.com/KNNSpeed/Simple-Kernel
//
// This program is a small x86-64 program for use with the Simple UEFI Bootloader: https://github.com/KNNSpeed/Simple-UEFI-Bootloader.
// It contains some functions that might prove useful in development of other bare-metal programs, and showcases some of the features
// provided by the bootloader (e.g. Multi-GPU framebuffer support).
//
// The main function of this program, defined as "kernel_main" in the accompanying compile scripts, is passed a pointer to the following
// structure from the bootloader:
/*
  typedef struct {
    EFI_MEMORY_DESCRIPTOR  *Memory_Map;   // The system memory map as an array of EFI_MEMORY_DESCRIPTOR structs
    EFI_RUNTIME_SERVICES   *RTServices;   // UEFI Runtime Services
    GPU_CONFIG             *GPU_Configs;  // Information about available graphics output devices; see below for details
    EFI_FILE_INFO          *FileMeta;     // Kernel64 file metadata
    void                   *RSDP;         // A pointer to the RSDP ACPI table
  } LOADER_PARAMS;
*/
//
// GPU_CONFIG is a custom structure that is defined as follows:
//
/*
  typedef struct {
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE  *GPUArray;             // An array of EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE structs defining each available framebuffer
    UINT64                              NumberOfFrameBuffers; // The number of structs in the array (== the number of available framebuffers)
  } GPU_CONFIG;
*/
//
// The header file, Kernel64.h, contains definitions for the data types of each pointer in the above structures for easy reference.
//
// Note: As mentioned in the bootloader's embedded documentation, GPU_Configs provides access to linear framebuffer addresses for directly
// drawing to connected screens: specifically one for each active display per GPU. Typically there is one active display per GPU, but it is
// up to the GPU firmware maker to deterrmine that. See "12.10 Rules for PCI/AGP Devices" in the UEFI Specification 2.7 Errata A for more
// details: http://www.uefi.org/specifications
//

#include "Kernel64.h"

#define MAJOR_VER 0
#define MINOR_VER 'x'

#define fontarray font8x8_basic // Must be set up in UTF-8

// The character print function can draw raw single-color bitmaps formatted like this, given appropriate height and width values
char load_image[48] = {
    0x00, 0x3F, 0x80, 0x00, // ........ ..@@@@@@ @....... ........
    0x01, 0x80, 0x30, 0x00, // .......@ @....... ..@@.... ........
    0x0C, 0x00, 0x06, 0x00, // ....@@.. ........ .....@@. ........
    0x30, 0x1E, 0xE1, 0x80, // ..@@.... ...@@@@. @@@....@ @.......
    0x60, 0x61, 0xC0, 0xC0, // .@@..... .@@....@ @@...... @@......
    0xC0, 0xC0, 0xC0, 0x60, // @@...... @@...... @@...... .@@.....
    0xC0, 0xC0, 0xE0, 0x60, // @@...... @@...... @@@..... .@@.....
    0x60, 0x61, 0xB0, 0xC0, // .@@..... .@@....@ @.@@.... @@......
    0x30, 0x1E, 0x1F, 0x80, // ..@@.... ...@@@@. ...@@@@@ @.......
    0x0C, 0x00, 0x00, 0x00, // ....@@.. ........ ........ ........
    0x01, 0x80, 0x3C, 0x00, // .......@ @....... ..@@@@.. ........
    0x00, 0x3F, 0x80, 0x00  // ........ ..@@@@@@ @....... ........
}; // Width = 27 bits, height = 12 bytes

// load_image2 is what actually looks like load_image's ascii art when rendered
char load_image2[96] = {
    0x00, 0x3F, 0x80, 0x00, // ........ ..@@@@@@ @....... ........
    0x00, 0x3F, 0x80, 0x00, // ........ ..@@@@@@ @....... ........
    0x01, 0x80, 0x30, 0x00, // .......@ @....... ..@@.... ........
    0x01, 0x80, 0x30, 0x00, // .......@ @....... ..@@.... ........
    0x0C, 0x00, 0x06, 0x00, // ....@@.. ........ .....@@. ........
    0x0C, 0x00, 0x06, 0x00, // ....@@.. ........ .....@@. ........
    0x30, 0x1E, 0xE1, 0x80, // ..@@.... ...@@@@. @@@....@ @.......
    0x30, 0x1E, 0xE1, 0x80, // ..@@.... ...@@@@. @@@....@ @.......
    0x60, 0x61, 0xC0, 0xC0, // .@@..... .@@....@ @@...... @@......
    0x60, 0x61, 0xC0, 0xC0, // .@@..... .@@....@ @@...... @@......
    0xC0, 0xC0, 0xC0, 0x60, // @@...... @@...... @@...... .@@.....
    0xC0, 0xC0, 0xC0, 0x60, // @@...... @@...... @@...... .@@.....
    0xC0, 0xC0, 0xE0, 0x60, // @@...... @@...... @@@..... .@@.....
    0xC0, 0xC0, 0xE0, 0x60, // @@...... @@...... @@@..... .@@.....
    0x60, 0x61, 0xB0, 0xC0, // .@@..... .@@....@ @.@@.... @@......
    0x60, 0x61, 0xB0, 0xC0, // .@@..... .@@....@ @.@@.... @@......
    0x30, 0x1E, 0x1F, 0x80, // ..@@.... ...@@@@. ...@@@@@ @.......
    0x30, 0x1E, 0x1F, 0x80, // ..@@.... ...@@@@. ...@@@@@ @.......
    0x0C, 0x00, 0x00, 0x00, // ....@@.. ........ ........ ........
    0x0C, 0x00, 0x00, 0x00, // ....@@.. ........ ........ ........
    0x01, 0x80, 0x3C, 0x00, // .......@ @....... ..@@@@.. ........
    0x01, 0x80, 0x3C, 0x00, // .......@ @....... ..@@@@.. ........
    0x00, 0x3F, 0x80, 0x00, // ........ ..@@@@@@ @....... ........
    0x00, 0x3F, 0x80, 0x00  // ........ ..@@@@@@ @....... ........
}; // Width = 27 bits, height = 24 bytes

char load_image3[144] = {
    0x00, 0x3F, 0x80, 0x00, // ........ ..@@@@@@ @....... ........
    0x00, 0x3F, 0x80, 0x00, // ........ ..@@@@@@ @....... ........
    0x00, 0x3F, 0x80, 0x00, // ........ ..@@@@@@ @....... ........
    0x01, 0x80, 0x30, 0x00, // .......@ @....... ..@@.... ........
    0x01, 0x80, 0x30, 0x00, // .......@ @....... ..@@.... ........
    0x01, 0x80, 0x30, 0x00, // .......@ @....... ..@@.... ........
    0x0C, 0x00, 0x06, 0x00, // ....@@.. ........ .....@@. ........
    0x0C, 0x00, 0x06, 0x00, // ....@@.. ........ .....@@. ........
    0x0C, 0x00, 0x06, 0x00, // ....@@.. ........ .....@@. ........
    0x30, 0x1E, 0xE1, 0x80, // ..@@.... ...@@@@. @@@....@ @.......
    0x30, 0x1E, 0xE1, 0x80, // ..@@.... ...@@@@. @@@....@ @.......
    0x30, 0x1E, 0xE1, 0x80, // ..@@.... ...@@@@. @@@....@ @.......
    0x60, 0x61, 0xC0, 0xC0, // .@@..... .@@....@ @@...... @@......
    0x60, 0x61, 0xC0, 0xC0, // .@@..... .@@....@ @@...... @@......
    0x60, 0x61, 0xC0, 0xC0, // .@@..... .@@....@ @@...... @@......
    0xC0, 0xC0, 0xC0, 0x60, // @@...... @@...... @@...... .@@.....
    0xC0, 0xC0, 0xC0, 0x60, // @@...... @@...... @@...... .@@.....
    0xC0, 0xC0, 0xC0, 0x60, // @@...... @@...... @@...... .@@.....
    0xC0, 0xC0, 0xE0, 0x60, // @@...... @@...... @@@..... .@@.....
    0xC0, 0xC0, 0xE0, 0x60, // @@...... @@...... @@@..... .@@.....
    0xC0, 0xC0, 0xE0, 0x60, // @@...... @@...... @@@..... .@@.....
    0x60, 0x61, 0xB0, 0xC0, // .@@..... .@@....@ @.@@.... @@......
    0x60, 0x61, 0xB0, 0xC0, // .@@..... .@@....@ @.@@.... @@......
    0x60, 0x61, 0xB0, 0xC0, // .@@..... .@@....@ @.@@.... @@......
    0x30, 0x1E, 0x1F, 0x80, // ..@@.... ...@@@@. ...@@@@@ @.......
    0x30, 0x1E, 0x1F, 0x80, // ..@@.... ...@@@@. ...@@@@@ @.......
    0x30, 0x1E, 0x1F, 0x80, // ..@@.... ...@@@@. ...@@@@@ @.......
    0x0C, 0x00, 0x00, 0x00, // ....@@.. ........ ........ ........
    0x0C, 0x00, 0x00, 0x00, // ....@@.. ........ ........ ........
    0x0C, 0x00, 0x00, 0x00, // ....@@.. ........ ........ ........
    0x01, 0x80, 0x3C, 0x00, // .......@ @....... ..@@@@.. ........
    0x01, 0x80, 0x3C, 0x00, // .......@ @....... ..@@@@.. ........
    0x01, 0x80, 0x3C, 0x00, // .......@ @....... ..@@@@.. ........
    0x00, 0x3F, 0x80, 0x00, // ........ ..@@@@@@ @....... ........
    0x00, 0x3F, 0x80, 0x00, // ........ ..@@@@@@ @....... ........
    0x00, 0x3F, 0x80, 0x00  // ........ ..@@@@@@ @....... ........
}; // Width = 27 bits, height = 36 bytes
// Output_render will ignore the last 5 bits of zeros in each row if width is specified as 27.

// The only data we can work with is what is passed into this function, plus the few std includes in the .h file.
// To print text requires a bitmap font.
//void kernel_main(EFI_MEMORY_DESCRIPTOR * Memory_Map, EFI_RUNTIME_SERVICES * RTServices, EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE * GPU_Mode, EFI_FILE_INFO * FileMeta, void * RSDP)
void kernel_main(LOADER_PARAMS * LP) // Loader Parameters
{
  // TODO: Need to use PixelsPerScanLine instead of horizontal resolution, and need to center horizontal resolution within PixelsPerScanLine
  /*
    typedef struct {
      EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE  *GPUArray; // This array contains the EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE structures for each available framebuffer
      UINT64                              NumberOfFrameBuffers; // The number of pointers in the array (== the number of available framebuffers)
    } GPU_CONFIG;

    typedef struct {
      EFI_MEMORY_DESCRIPTOR  *Memory_Map;
      EFI_RUNTIME_SERVICES   *RTServices;
      GPU_CONFIG             *GPU_Configs;
      EFI_FILE_INFO          *FileMeta;
      void                   *RSDP;
    } LOADER_PARAMS;
  */

  /*
  typedef UINT64          EFI_PHYSICAL_ADDRESS;
  typedef UINT64          EFI_VIRTUAL_ADDRESS;

  #define EFI_MEMORY_DESCRIPTOR_VERSION  1
  typedef struct {
      UINT32                          Type;           // Field size is 32 bits followed by 32 bit pad
      UINT32                          Pad;            // There's no pad in the spec...
      EFI_PHYSICAL_ADDRESS            PhysicalStart;  // Field size is 64 bits
      EFI_VIRTUAL_ADDRESS             VirtualStart;   // Field size is 64 bits
      UINT64                          NumberOfPages;  // Field size is 64 bits
      UINT64                          Attribute;      // Field size is 64 bits
  } EFI_MEMORY_DESCRIPTOR;
  */
//  int wait = 1;
  uint32_t iterator = 1;
  char swapped_image[sizeof(load_image2)] = {0}; // You never know what's lurking in memory that hasn't been cleaned up...
  // In this case UEFI stuff. This program doesn't care about anything not passed into it explicitly, so as long as we don't
  // accidentally overwrite things in the loader block, we'll be OK. Really should be consulting/checking against the memmap to be sure, though.
  //... Would be much easier to print the whole memmap, once formatted string printing works.

  //TODO: Check what this swapped_image buffer is running over. There's something under here. Requires working printf or equivalent (or, well, some kind of basic memory management)

//  bitmap_bitswap(load_image, 12, 27, swapped_image);
//  char swapped_image2[sizeof(load_image)];
//  bitmap_bytemirror(swapped_image, 12, 27, swapped_image2);
  bitmap_bitreverse(load_image2, 24, 27, swapped_image);
  for(UINT64 k = 0; k < LP->GPU_Configs->NumberOfFrameBuffers; k++) // Multi-GPU support!
  {
    single_char_anywhere_scaled(LP->GPU_Configs->GPUArray[k].FrameBufferBase, swapped_image, 24, 27, LP->GPU_Configs->GPUArray[k].Info->HorizontalResolution, LP->GPU_Configs->GPUArray[k].Info->VerticalResolution, 0x0000FFFF, 0xFF000000, (LP->GPU_Configs->GPUArray[k].Info->HorizontalResolution - 5*27)/2, (LP->GPU_Configs->GPUArray[k].Info->VerticalResolution - 5*24)/2, 5);
  }

  while(iterator) // loop ends on overflow
  {
//    asm volatile ("pause");
    asm volatile ("nop");
    iterator++; // Count to 2^32, which is about 1 second at 4 GHz. 2^64 is... still insanely long.
  }
  iterator = 1;

  Colorscreen(LP->GPU_Configs->GPUArray[0].FrameBufferBase, LP->GPU_Configs->GPUArray[0].Info->HorizontalResolution, LP->GPU_Configs->GPUArray[0].Info->VerticalResolution, 0x000000FF); // Blue in BGRX (X = reserved, technically an "empty alpha channel" for 32-bit memory alignment)
  single_char(LP->GPU_Configs->GPUArray[0].FrameBufferBase, fontarray['?'], 8, 8, LP->GPU_Configs->GPUArray[0].Info->HorizontalResolution, LP->GPU_Configs->GPUArray[0].Info->VerticalResolution, 0x00FFFFFF, 0x00000000);
  single_char_anywhere(LP->GPU_Configs->GPUArray[0].FrameBufferBase, fontarray['!'], 8, 8, LP->GPU_Configs->GPUArray[0].Info->HorizontalResolution, LP->GPU_Configs->GPUArray[0].Info->VerticalResolution, 0x00FFFFFF, 0xFF000000, LP->GPU_Configs->GPUArray[0].Info->HorizontalResolution/4, LP->GPU_Configs->GPUArray[0].Info->VerticalResolution/3);
  single_char_anywhere_scaled(LP->GPU_Configs->GPUArray[0].FrameBufferBase, fontarray['H'], 8, 8, LP->GPU_Configs->GPUArray[0].Info->HorizontalResolution, LP->GPU_Configs->GPUArray[0].Info->VerticalResolution, 0x00FFFFFF, 0xFF000000, 10, 10, 5);
  string_anywhere_scaled(LP->GPU_Configs->GPUArray[0].FrameBufferBase, "Is it soup?", 8, 8, LP->GPU_Configs->GPUArray[0].Info->HorizontalResolution, LP->GPU_Configs->GPUArray[0].Info->VerticalResolution, 0x00FFFFFF, 0x00000000, 10, 10, 1);
  while(iterator) // loop ends on overflow
  {
//    asm volatile ("pause");
    asm volatile ("nop");
    iterator++; // Count to 2^32, which is about 1 second at 4 GHz. 2^64 is... still insanely long.
  }
  iterator = 1;

  Colorscreen(LP->GPU_Configs->GPUArray[0].FrameBufferBase, LP->GPU_Configs->GPUArray[0].Info->HorizontalResolution, LP->GPU_Configs->GPUArray[0].Info->VerticalResolution, 0x0000FF00); // Green in BGRX (X = reserved, technically an "empty alpha channel" for 32-bit memory alignment)
  single_char(LP->GPU_Configs->GPUArray[0].FrameBufferBase, fontarray['A'], 8, 8, LP->GPU_Configs->GPUArray[0].Info->HorizontalResolution, LP->GPU_Configs->GPUArray[0].Info->VerticalResolution, 0x00FFFFFF, 0x00000000);
  single_char_anywhere(LP->GPU_Configs->GPUArray[0].FrameBufferBase, fontarray['!'], 8, 8, LP->GPU_Configs->GPUArray[0].Info->HorizontalResolution, LP->GPU_Configs->GPUArray[0].Info->VerticalResolution, 0x00FFFFFF, 0xFF000000, LP->GPU_Configs->GPUArray[0].Info->HorizontalResolution/4, LP->GPU_Configs->GPUArray[0].Info->VerticalResolution/3);
  string_anywhere_scaled(LP->GPU_Configs->GPUArray[0].FrameBufferBase, "Is it really soup?", 8, 8, LP->GPU_Configs->GPUArray[0].Info->HorizontalResolution, LP->GPU_Configs->GPUArray[0].Info->VerticalResolution, 0x00FFFFFF, 0x00000000, 50, 50, 3);
  while(iterator) // loop ends on overflow
  {
//    asm volatile ("pause");
    asm volatile ("nop");
    iterator++; // Count to 2^32, which is about 1 second at 4 GHz. 2^64 is... still insanely long.
  }
  iterator = 1;

  Colorscreen(LP->GPU_Configs->GPUArray[0].FrameBufferBase, LP->GPU_Configs->GPUArray[0].Info->HorizontalResolution, LP->GPU_Configs->GPUArray[0].Info->VerticalResolution, 0x00FF0000); // Red in BGRX (X = reserved, technically an "empty alpha channel" for 32-bit memory alignment)
  single_char(LP->GPU_Configs->GPUArray[0].FrameBufferBase, fontarray['2'], 8, 8, LP->GPU_Configs->GPUArray[0].Info->HorizontalResolution, LP->GPU_Configs->GPUArray[0].Info->VerticalResolution, 0x00FFFFFF, 0xFF000000);
  while(iterator) // loop ends on overflow
  {
//    asm volatile ("pause");
    asm volatile ("nop");
    iterator++; // Count to 2^32, which is about 1 second at 4 GHz. 2^64 is... still insanely long.
  }
  iterator = 1;

  Blackscreen(LP->GPU_Configs->GPUArray[0].FrameBufferBase, LP->GPU_Configs->GPUArray[0].Info->HorizontalResolution, LP->GPU_Configs->GPUArray[0].Info->VerticalResolution); // X in BGRX (X = reserved, technically an "empty alpha channel" for 32-bit memory alignment)
  single_pixel(LP->GPU_Configs->GPUArray[0].FrameBufferBase, LP->GPU_Configs->GPUArray[0].Info->HorizontalResolution / 2, LP->GPU_Configs->GPUArray[0].Info->VerticalResolution / 2, LP->GPU_Configs->GPUArray[0].Info->HorizontalResolution, LP->GPU_Configs->GPUArray[0].Info->VerticalResolution, 0x00FFFFFF);
  single_char(LP->GPU_Configs->GPUArray[0].FrameBufferBase, fontarray['@'], 8, 8, LP->GPU_Configs->GPUArray[0].Info->HorizontalResolution, LP->GPU_Configs->GPUArray[0].Info->VerticalResolution, 0x00FFFFFF, 0x00000000);
  single_char_anywhere(LP->GPU_Configs->GPUArray[0].FrameBufferBase, fontarray['!'], 8, 8, LP->GPU_Configs->GPUArray[0].Info->HorizontalResolution, LP->GPU_Configs->GPUArray[0].Info->VerticalResolution, 0x00FFFFFF, 0xFF000000, 512, 512);
  single_char_anywhere_scaled(LP->GPU_Configs->GPUArray[0].FrameBufferBase, fontarray['I'], 8, 8, LP->GPU_Configs->GPUArray[0].Info->HorizontalResolution, LP->GPU_Configs->GPUArray[0].Info->VerticalResolution, 0x00FFFFFF, 0xFF000000, 10, 10, 2);
  string_anywhere_scaled(LP->GPU_Configs->GPUArray[0].FrameBufferBase, "OMG it's actually soup! I don't believe it!!", 8, 8, LP->GPU_Configs->GPUArray[0].Info->HorizontalResolution, LP->GPU_Configs->GPUArray[0].Info->VerticalResolution, 0x00FFFFFF, 0x00000000, 0,  LP->GPU_Configs->GPUArray[0].Info->VerticalResolution/2, 2);
  while(iterator) // loop ends on overflow
  {
//    asm volatile ("pause");
    asm volatile ("nop");
    iterator++; // Count to 2^32, which is about 1 second at 4 GHz. 2^64 is... still insanely long.
  }

  LP->RTServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL); // Shutdown the system
}

void Blackscreen(EFI_PHYSICAL_ADDRESS lfb_base_addr, UINT32 HREZ, UINT32 VREZ)
{
/*
  UINT32 row, col;
  for (row = 0; row < VREZ; row++)
  {
    for (col = 0; col < HREZ; col++)
    {
      *(UINT32*)lfb_base_addr = 0x00000000;
      lfb_base_addr+=4; // 4 Bpp == 32 bpp
    }
  }
  */
  Colorscreen(lfb_base_addr, HREZ, VREZ, 0x00000000);
}

void Colorscreen(EFI_PHYSICAL_ADDRESS lfb_base_addr, UINT32 HREZ, UINT32 VREZ, UINT32 color)
{
  UINT32 row, col;
  for (row = 0; row < VREZ; row++)
  {
    for (col = 0; col < HREZ; col++)
    {
      *(UINT32*)lfb_base_addr = color; // The thing at lfb_base_addr is an address pointing to UINT32s. lfb_base_addr itself is a 64-bit number.
      lfb_base_addr+=4; // 4 Bpp == 32 bpp
    }
  }
}


void single_pixel(EFI_PHYSICAL_ADDRESS lfb_base_addr, UINT32 x, UINT32 y, UINT32 HREZ, UINT32 VREZ, UINT32 color)
{
  if(y > VREZ || x > HREZ) // Need some kind of error indicator (makes screen red)
  {
    Colorscreen(lfb_base_addr, HREZ, VREZ, 0x00FF0000);
  }

//  *(UINT32*)(lfb_base_addr + y * 4 * HREZ + x * 4) = color;
  char one = 0x01;
  Output_render(lfb_base_addr, &one, 1, 1, HREZ, VREZ, color, 0x00000000, x, y, 1, 0, 0, 0);
}

void single_char(EFI_PHYSICAL_ADDRESS lfb_base_addr, char * character, UINT32 height, UINT32 width, UINT32 HREZ, UINT32 VREZ, UINT32 font_color, UINT32 highlight_color)
{
  // Height in number of bytes and width in number of bits
  // Assuming "character" is an array of bytes, e.g. { 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00},   // U+0040 (@), which is an 8x8 '@' sign
  if(height > VREZ || width > HREZ)
  {
    Colorscreen(lfb_base_addr, HREZ, VREZ, 0x00FF0000); // Need some kind of error indicator (makes screen red)
  } // Could use an instruction like ARM's USAT to truncate values

/*
  //mapping: x*4 + y*4*(HREZ), x is column number, y is row number; every 4*HREZ bytes is a new on-screen row
  // y max is VREZ, x max is HREZ, 'x4' is the LFB expansion, since 1 bit input maps to 4 bytes output
  for(int row = 0; row < height; row++) // for byte in row
  {
    for (int bit = 0; bit < width; bit++) // for bit in column
    {
      if((character[row] >> bit) & 0x1) // if a 1, output
      {
        if(*(UINT32*)(lfb_base_addr + row*4*HREZ + bit*4) != font_color) // No need to write if it's already there
        {
          *(UINT32*)(lfb_base_addr + row*4*HREZ + bit*4) = font_color;
        }
      }
      else // if a 0, background or skip (set highlight_color to 0xFF000000 for transparency)
      {
        if((highlight_color != 0xFF000000) && (*(UINT32*)(lfb_base_addr + row*4*HREZ + bit*4) != highlight_color))
        {
          *(UINT32*)(lfb_base_addr + row*4*HREZ + bit*4) = highlight_color;
        }
        // do nothing if color's already there for speed
      }
    }
  }
*/

  Output_render(lfb_base_addr, character, height, width, HREZ, VREZ, font_color, highlight_color, 0, 0, 1, 0, 0, 0);
}

void single_char_anywhere(EFI_PHYSICAL_ADDRESS lfb_base_addr, char * character, UINT32 height, UINT32 width, UINT32 HREZ, UINT32 VREZ, UINT32 font_color, UINT32 highlight_color, UINT32 x, UINT32 y)
{
  // x and y are top leftmost coordinates
  // Height in number of bytes and width in number of bits
  // Assuming "character" is an array of bytes, e.g. { 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00},   // U+0040 (@), which is an 8x8 '@' sign
  if(height > VREZ || width > HREZ)
  {
    Colorscreen(lfb_base_addr, HREZ, VREZ, 0x00FF0000); // Need some kind of error indicator (makes screen red)
  } // Could use an instruction like ARM's USAT to truncate values
  else if(x > HREZ || y > VREZ)
  {
    Colorscreen(lfb_base_addr, HREZ, VREZ, 0x0000FF00); // Makes screen green
  }
  else if ((y + height) > VREZ || (x + width) > HREZ)
  {
    Colorscreen(lfb_base_addr, HREZ, VREZ, 0x000000FF); // Makes screen blue
  }

/*
  //mapping: x*4 + y*4*(HREZ), x is column number, y is row number; every 4*HREZ bytes is a new on-screen row
  // y max is VREZ, x max is HREZ, 'x4' is the LFB expansion, since 1 bit input maps to 4 bytes output
  for(int row = 0; row < height; row++) // for byte in row
  {
    for (int bit = 0; bit < width; bit++) // for bit in column
    {
      if((character[row] >> bit) & 0x1) // if a 1, output
      {
        if(*(UINT32*)(lfb_base_addr + (y + row)*4*HREZ + (x + bit)*4) != font_color) // No need to write if it's already there
        {
          *(UINT32*)(lfb_base_addr + (y + row)*4*HREZ + (x + bit)*4) = font_color;
        }
      }
      else // if a 0, background or skip (set highlight_color to 0xFF000000 for transparency)
      {
        if((highlight_color != 0xFF000000) && (*(UINT32*)(lfb_base_addr + (y + row)*4*HREZ + (x + bit)*4) != highlight_color))
        {
          *(UINT32*)(lfb_base_addr + (y + row)*4*HREZ + (x + bit)*4) = highlight_color;
        }
        // do nothing if color's already there for speed
      }
    }
  }
*/

  Output_render(lfb_base_addr, character, height, width, HREZ, VREZ, font_color, highlight_color, x, y, 1, 0, 0, 0);
}
// You know, this could probably do images. just pass in an appropriately-formatted array of bytes as the 'character' pointer. Arbitrarily-sized fonts should work, too.
// This is probably the slowest possible way of doing what this function does, though, since it's all based on absolute addressing.
void single_char_anywhere_scaled(EFI_PHYSICAL_ADDRESS lfb_base_addr, char * character, UINT32 height, UINT32 width, UINT32 HREZ, UINT32 VREZ, UINT32 font_color, UINT32 highlight_color, UINT32 x, UINT32 y, UINT32 scale)
{
  // scale is an integer scale factor, e.g. 2 for 2x, 3 for 3x, etc.
  // x and y are top leftmost coordinates
  // Height in number of bytes and width in number of bits
  // Assuming "character" is an array of bytes, e.g. { 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00},   // U+0040 (@), which is an 8x8 '@' sign
  if(height > VREZ || width > HREZ)
  {
    Colorscreen(lfb_base_addr, HREZ, VREZ, 0x00FF0000); // Need some kind of error indicator (makes screen red)
  } // Could use an instruction like ARM's USAT to truncate values
  else if(x > HREZ || y > VREZ)
  {
    Colorscreen(lfb_base_addr, HREZ, VREZ, 0x0000FF00); // Makes screen green
  }
  else if ((y + scale*height) > VREZ || (x + scale*width) > HREZ)
  {
    Colorscreen(lfb_base_addr, HREZ, VREZ, 0x000000FF); // Makes screen blue
  }

/*
  //mapping: x*4 + y*4*(HREZ), x is column number, y is row number; every 4*HREZ bytes is a new on-screen row
  // y max is VREZ, x max is HREZ, 'x4' is the LFB expansion, since 1 bit input maps to 4 bytes output
  // A 2x scale should turn 1 pixel into a square of 2x2 pixels
  for(int row = 0; row < height; row++) // for byte in row
  {
    for (int bit = 0; bit < width; bit++) // for bit in column
    {
      if((character[row] >> bit) & 0x1) // if a 1, output
      {
// start scale here
        for(int b = 0; b < scale; b++)
        {
          for(int a = 0; a < scale; a++)
          {
            if(*(UINT32*)(lfb_base_addr + ((y*HREZ + x) + scale*(row*HREZ + bit) + (b*HREZ + a))*4) != font_color) // No need to write if it's already there
            {
              *(UINT32*)(lfb_base_addr + ((y*HREZ + x) + scale*(row*HREZ + bit) + (b*HREZ + a))*4) = font_color;
            }
          }
        }
//end scale here
      }
      else // if a 0, background or skip (set highlight_color to 0xFF000000 for transparency)
      {
// start scale here
        for(int b = 0; b < scale; b++)
        {
          for(int a = 0; a < scale; a++)
          {
            if((highlight_color != 0xFF000000) && (*(UINT32*)(lfb_base_addr + ((y*HREZ + x) + scale*(row*HREZ + bit) + (b*HREZ + a))*4) != highlight_color))
            {
              *(UINT32*)(lfb_base_addr + ((y*HREZ + x) + scale*(row*HREZ + bit) + (b*HREZ + a))*4) = highlight_color;
            }
            // do nothing if color's already there for speed
          }
        }
//end scale here
      }
    }
  }
*/

  Output_render(lfb_base_addr, character, height, width, HREZ, VREZ, font_color, highlight_color, x, y, scale, 0, 0, 0);
}

// literal strings in C are automatically null-terminated. i.e. "hi" is actually 3 characters long.
void string_anywhere_scaled(EFI_PHYSICAL_ADDRESS lfb_base_addr, char * string, UINT32 height, UINT32 width, UINT32 HREZ, UINT32 VREZ, UINT32 font_color, UINT32 highlight_color, UINT32 x, UINT32 y, UINT32 scale)
{
  // scale is an integer scale factor, e.g. 2 for 2x, 3 for 3x, etc.
  // x and y are top leftmost coordinates
  // Height in number of bytes and width in number of bits, per character
  // Assuming "character" is an array of bytes, e.g. { 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00},   // U+0040 (@), which is an 8x8 '@' sign
  if(height > VREZ || width > HREZ)
  {
    Colorscreen(lfb_base_addr, HREZ, VREZ, 0x00FF0000); // Need some kind of error indicator (makes screen red)
  } // Could use an instruction like ARM's USAT to truncate values
  else if(x > HREZ || y > VREZ)
  {
    Colorscreen(lfb_base_addr, HREZ, VREZ, 0x0000FF00); // Makes screen green
  }
  else if ((y + scale*height) > VREZ || (x + scale*width) > HREZ)
  {
    Colorscreen(lfb_base_addr, HREZ, VREZ, 0x000000FF); // Makes screen blue
  }

  //mapping: x*4 + y*4*(HREZ), x is column number, y is row number; every 4*HREZ bytes is a new on-screen row
  // y max is VREZ, x max is HREZ, 'x4' is the LFB expansion, since 1 bit input maps to 4 bytes output

  // A 2x scale should turn 1 pixel into a square of 2x2 pixels
// iterate through characters in string
// start while
  uint32_t item = 0;
  char * character;
  while(string[item] != '\0') // for char in string until \0
  {
    character = fontarray[(int)string[item]]; // match the character to the font, using UTF-8.
    // This would expect a font array to include everything in the UTF-8 character set... Or just use the most common ones.

    // Output render
    /*
    for(int row = 0; row < height; row++) // for number of rows
    {
      for (int bit = 0; bit < width; bit++) // for bit in column
      {
        if((character[row] >> bit) & 0x1) // if a 1, output
        {
          // start scale here
          for(int b = 0; b < scale; b++)
          {
            for(int a = 0; a < scale; a++)
            {
              if(*(UINT32*)(lfb_base_addr + ((y*HREZ + x) + scale*(row*HREZ + bit) + (b*HREZ + a) + scale * item * width)*4) != font_color) // No need to write if it's already there
              {
                *(UINT32*)(lfb_base_addr + ((y*HREZ + x) + scale*(row*HREZ + bit) + (b*HREZ + a) + scale * item * width)*4) = font_color;
              }
            }
          } //end scale here
        } //end if
        else // if a 0, background or skip (set highlight_color to 0xFF000000 for transparency)
        {
          // start scale here
          for(int b = 0; b < scale; b++)
          {
            for(int a = 0; a < scale; a++)
            {
              if((highlight_color != 0xFF000000) && (*(UINT32*)(lfb_base_addr + ((y*HREZ + x) + scale*(row*HREZ + bit) + (b*HREZ + a) + scale * item * width)*4) != highlight_color))
              {
                *(UINT32*)(lfb_base_addr + ((y*HREZ + x) + scale*(row*HREZ + bit) + (b*HREZ + a) + scale * item * width)*4) = highlight_color;
              }
              // do nothing if color's already there for speed
            }
          } //end scale here
        } // end else
      } // end bit in column
    } // end byte in row
    // End output render
    */
    Output_render(lfb_base_addr, character, height, width, HREZ, VREZ, font_color, highlight_color, x, y, scale, item, 0, 0);
    item++;
  } // end while
} // end function

//////////////////////////////////////////////////////
/*
void formatted_string_anywhere_scaled(EFI_PHYSICAL_ADDRESS lfb_base_addr, char * string, UINT32 height, UINT32 width, UINT32 HREZ, UINT32 VREZ, UINT32 font_color, UINT32 highlight_color, UINT32 x, UINT32 y, UINT32 scale)
{
  // scale is an integer scale factor, e.g. 2 for 2x, 3 for 3x, etc.
  // x and y are top leftmost coordinates
  // Height in number of bytes and width in number of bits, per character
  // Assuming "character" is an array of bytes, e.g. { 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00},   // U+0040 (@), which is an 8x8 '@' sign
  if(height > VREZ || width > HREZ)
  {
    Colorscreen(lfb_base_addr, HREZ, VREZ, 0x00FF0000); // Need some kind of error indicator (makes screen red)
  } // Could use an instruction like ARM's USAT to truncate values
  else if(x > HREZ || y > VREZ)
  {
    Colorscreen(lfb_base_addr, HREZ, VREZ, 0x0000FF00); // Makes screen green
  }
  else if ((y + scale*height) > VREZ || (x + scale*width) > HREZ)
  {
    Colorscreen(lfb_base_addr, HREZ, VREZ, 0x000000FF); // Makes screen blue
  }

  //mapping: x*4 + y*4*(HREZ), x is column number, y is row number; every 4*HREZ bytes is a new on-screen row
  // y max is VREZ, x max is HREZ, 'x4' is the LFB expansion, since 1 bit input maps to 4 bytes output

  // A 2x scale should turn 1 pixel into a square of 2x2 pixels
// iterate through characters in string
// start while
  uint32_t item = 0;
  uint32_t formatting_subtractor = 0; // need to keep track of formatting characters so the printed output doesn't get messed up
  uint32_t formatting_adder = 0; // need to keep track of formatting characters so the printed output doesn't get messed up
  int format_end;
  int padding = 0;
  int pad_after = 0;
  int precision_dot = 0;
  char * character;
  char * formatbuffer[4096]; // Max size a string can be

  while(string[item] != '\0') // for char in string until \0
  {
    character = fontarray[(int)string[item]]; // match the character to the font, using UTF-8.
    if(character == '%') // Start dealing with formatted part, before going back to character printing
    {
      //formatted string, check what comes next and iterate "item" until out of formatting code
      formatting_subtractor++; // To account for the first "%"
      format_end = 0;
      while(!format_end) //TODO: What printf and co. do is build a string first and then output it. that's probably what should happen here, too: build a string and then send it to string_anywhere_scaled.
      // ...Or we can just do it asynchronously, printing out whatever we get as we get it whilst respecting formatting.
      // That way, Output_render can be used to output to the screen as asynchronously as possible, but scanf-like functionality can be set with a flag to write to a buffer instead.
      // This would enable per-character scanf right into printing with a toggle, as well as mirroring whatever gets output
      // Ergo formatted_string_anywhere_scaled_io(... UINT32 input_output), if input_output = 0, output (printf). if = 1, input (scanf). if = 2, both.
      // Can also make, e.g. %m12, next 12 characters get scanf'd instead of printed, but print the rest
      {
        character = fontarray[(int)string[++item]]; // This "format mode" will only be true for as long as the valid formatting string lasts
        switch(character) // We DO have stdarg, so we can use va_arg!
        {
          case '%': //done
              //
              // %% -> %
              //

              // Keep on going and print the % like normal, with the output offset by 1 for the first %
              format_end = 1;
              break; // This breaks the switch statement, but a continue will go back to the while loop.

          case '0': //done
              // need to know how many zeroes there are
              padding = 1;
              formatting_subtractor++;
              break; // Go back to start of while loop to check the next value

          case '-':
              pad_after = 1;
              formatting_subtractor++;
              break;

          case '.':
              precision_dot = 1;
              formatting_subtractor++;
              break;

          case '*':
              *Item.WidthParse = va_arg(ps->args, UINTN);
              formatting_subtractor++;
              break;

          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
              formatting_subtractor++;
              if(padding == 1) // TODO: padding_size and formatting_size are supposed to be minimum widths for the whole modifier--right now this gives fixed padding
              {
                character = '0'; // this won't work. need fontarray
                padding = 0;
              }
              else
              {
                character = ' ';
              }

              int padding_size = *(int)character - '0'; // *(int)character is the same as (int)character[0]
              item++;
              while(string[item] <= '9' && string[item] >= '0') // check the next item
              {
                padding_size = padding_size * 10 + ((int)string[item] - '0');
                item++;
                formatting_subtractor++;
              }

              //padding_size -= [length of next thing] // next thing will either be 8, 16, 32, or 64-bit. either hhu, hu, u, or lu (llu == lu)
              for(int i = 0; i < padding_size; i++) //pad with zeros
              {
                Output_render(lfb_base_addr, character, height, width, HREZ, VREZ, font_color, highlight_color, x, y, scale, --item, formatting_subtractor, formatting_adder); // de-increment item so that the next character is correct and to not screw up the padding
              }
              formatting_adder += padding_size;
//              formatting_adder += (padding_size + [length of next thing]); // Don't need this if "next thing" truly gets evaluated next and not in this case statement
              continue; // need to check for x or d or something next

          case 'a':
              Item.Item.pc = va_arg(ps->args, CHAR8 *);
              Item.Item.Ascii = TRUE;
              if (!Item.Item.pc) {
                  Item.Item.pc = (CHAR8 *)"(null)";
              }
              formatting_subtractor++;
              break;

          case 's':
              Item.Item.pw = va_arg(ps->args, CHAR16 *);
              if (!Item.Item.pw) {
                  Item.Item.pw = L"(null)";
              }
              formatting_subtractor++;
              break;

          case 'c':
              Item.Scratch[0] = (CHAR16) va_arg(ps->args, UINTN);
              Item.Scratch[1] = 0;
              Item.Item.pw = Item.Scratch;
              formatting_subtractor++;
              break;

          case 'l':
              Item.Long = TRUE;
              formatting_subtractor++;
              break;

          case 'X': // should be uppercase hex
              Item.Width = Item.Long ? 16 : 8;
              Item.Pad = '0';
              formatting_subtractor++;
              break;
          case 'x':
              ValueToHex (
                  Item.Scratch,
                  Item.Long ? va_arg(ps->args, UINT64) : va_arg(ps->args, UINT32)
                  );
              Item.Item.pw = Item.Scratch;
              formatting_subtractor++;
              break;

          case 'G':
              GuidToString (Item.Scratch, va_arg(ps->args, EFI_GUID *));
              Item.Item.pw = Item.Scratch;
              formatting_subtractor++;
              break;

          case 'g':
              GuidToString (Item.Scratch, va_arg(ps->args, EFI_GUID *));
              Item.Item.pw = Item.Scratch;
              formatting_subtractor++;
              break;

          case 'u':
              ValueToString (
                  Item.Scratch,
                  Item.Comma,
                  Item.Long ? va_arg(ps->args, UINT64) : va_arg(ps->args, UINT32)
                  );
              Item.Item.pw = Item.Scratch;
              formatting_subtractor++;
              break;

          case 'd':
              ValueToString (
                  Item.Scratch,
                  Item.Comma,
                  Item.Long ? va_arg(ps->args, INT64) : va_arg(ps->args, INT32)
                  );
              Item.Item.pw = Item.Scratch;
              formatting_subtractor++;
              break;

          case 'f':
              FloatToString (
                  Item.Scratch,
                  Item.Comma,
                  va_arg(ps->args, double)
                  );
              Item.Item.pw = Item.Scratch;
              formatting_subtractor++;
              break;

          case 'n':
              PSETATTR(ps, ps->AttrNorm);
              formatting_subtractor++;
              break;

          case 'h':
              PSETATTR(ps, ps->AttrHighlight);
              formatting_subtractor++;
              break;

          case 'e':
              PSETATTR(ps, ps->AttrError);
              formatting_subtractor++;
              break;

          case 'N':
              Attr = ps->AttrNorm;
              break;

          case 'i':
              Attr = ps->AttrHighlight;
              formatting_subtractor++;
              break;

          case 'E':
              Attr = ps->AttrError;
              formatting_subtractor++;
              break;

          default:
              character = fontarray['?'];
              format_end = 1;
              break;
        } //end switch
      } //end while !format_end
      // add null-terminator '\0' to format_string
      //output formatted part here
      //string_anywhere_scaled(lfb_base_addr, format_string, height, width, HREZ, VREZ, font_color, highlight_color, x, y, scale);
    }// end % and continue printing the string

    Output_render(lfb_base_addr, character, height, width, HREZ, VREZ, font_color, highlight_color, x, y, scale, item, formatting_subtractor, formatting_adder);
    item++;
  } // end while
} // end function
*/
//////////////////////////////////////////////////////

void Output_render(EFI_PHYSICAL_ADDRESS lfb_base_addr, char * character, UINT32 height, UINT32 width, UINT32 HREZ, UINT32 VREZ, UINT32 font_color, UINT32 highlight_color, UINT32 x, UINT32 y, UINT32 scale, UINT32 item, UINT32 formatting_subtractor, UINT32 formatting_adder)
{
  // Compact ceiling function, so that size doesn't need to be passed in
  // This should be faster than a divide followed by a mod
  uint32_t row_iterator = width >> 3;
  if((width & 0x7) != 0)
  {
    row_iterator++;
  } // Width should never be zero, so the iterator will always be at least 1
  uint32_t i;

  for(uint32_t row = 0; row < height; row++) // for number of rows
  {
    i = 0;
    for (uint32_t bit = 0; bit < width; bit++) // for bit in column
    {
      if(((bit & 0x7) == 0 ) && (bit > 0))
      {
        i++;
      }
      // if a 1, output NOTE: There's gotta be a better way to do this.
//      if( character[row*row_iterator + i] & (1 << (7 - (bit & 0x7))) )
      if((character[row*row_iterator + i] >> (bit & 0x7)) & 0x1)
      {
        // start scale here
        for(uint32_t b = 0; b < scale; b++)
        {
          for(uint32_t a = 0; a < scale; a++)
          {
            if(*(UINT32*)(lfb_base_addr + ((y*HREZ + x) + scale*(row*HREZ + bit) + (b*HREZ + a) + scale * (item - formatting_subtractor + formatting_adder) * width)*4) != font_color) // No need to write if it's already there
            {
              *(UINT32*)(lfb_base_addr + ((y*HREZ + x) + scale*(row*HREZ + bit) + (b*HREZ + a) + scale * (item - formatting_subtractor + formatting_adder) * width)*4) = font_color;
            }
          }
        } //end scale here
      } //end if
      else // if a 0, background or skip (set highlight_color to 0xFF000000 for transparency)
      {
        // start scale here
        for(uint32_t b = 0; b < scale; b++)
        {
          for(uint32_t a = 0; a < scale; a++)
          {
            if((highlight_color != 0xFF000000) && (*(UINT32*)(lfb_base_addr + ((y*HREZ + x) + scale*(row*HREZ + bit) + (b*HREZ + a) + scale * (item - formatting_subtractor + formatting_adder) * width)*4) != highlight_color))
            {
              *(UINT32*)(lfb_base_addr + ((y*HREZ + x) + scale*(row*HREZ + bit) + (b*HREZ + a) + scale * (item - formatting_subtractor + formatting_adder) * width)*4) = highlight_color;
            }
            // do nothing if color's already there for speed
          }
        } //end scale here
      } // end else
    } // end bit in column
  } // end byte in row
}

void bitmap_bitswap(char * bitmap, UINT32 height, UINT32 width, char * output)
{
  uint32_t row_iterator = width >> 3;
  if((width & 0x7) != 0)
  {
    row_iterator++;
  } // Width should never be zero, so the iterator will always be at least 1

  for(uint32_t iter = 0; iter < height*row_iterator; iter++) // flip one byte at a time
  {
    output[iter] = (((bitmap[iter] >> 4) & 0xF) | ((bitmap[iter] & 0xF) << 4));
  }
}

// will mirror each byte in an array
void bitmap_bitreverse(char * bitmap, UINT32 height, UINT32 width, char * output)
{
  uint32_t row_iterator = width >> 3;
  if((width & 0x7) != 0)
  {
    row_iterator++;
  } // Width should never be zero, so the iterator will always be at least 1

  for(uint32_t iter = 0; iter < height*row_iterator; iter++) // flip one byte at a time
  {
    for(uint32_t bit = 0; bit < 8; bit++)
    {
      if( bitmap[iter] & (1 << (7 - bit)) )
      {
        output[iter] += (1 << bit);
      }
    }
  }
}


//Requires rectangular arrays, will create reflection of array
void bitmap_bytemirror(char * bitmap, UINT32 height, UINT32 width, char * output) // width in bits, height in bytes
{
  uint32_t row_iterator = width >> 3;
  if((width & 0x7) != 0)
  {
    row_iterator++;
  } // Width should never be zero, so the iterator will always be at least 1

  uint32_t iter, parallel_iter;
  if(row_iterator & 0x01)
  {//odd number of bytes per row
    for(iter = 0, parallel_iter = row_iterator - 1; (iter + (row_iterator >> 1) + 1) < height*row_iterator; iter++, parallel_iter--) // flip one byte at a time
    {
      if(iter == parallel_iter) // integer divide, (iter%row_iterator == row_iterator/2 - 1)
      {
        iter += (row_iterator >> 1) + 1; // hop the middle byte
        parallel_iter = iter + row_iterator - 1;
      }

      output[iter] = bitmap[parallel_iter]; // parallel_iter must mirror iter
      output[parallel_iter] = bitmap[iter];
    }
  }
  else
  {//even
    for(iter = 0, parallel_iter = row_iterator - 1; (iter + (row_iterator >> 1)) < height*row_iterator; iter++, parallel_iter--) // flip one byte at a time
    {
      if(iter - 1 == parallel_iter) // integer divide, (iter%row_iterator == row_iterator/2 - 1)
      {
        iter += (row_iterator >> 1); // skip to the next row to swap
        parallel_iter = iter + row_iterator - 1; // appropriately position parallel_iter based on iter
      }

      output[iter] = bitmap[parallel_iter]; // parallel_iter must mirror iter
      output[parallel_iter] = bitmap[iter];
    }
  }
}

////////////////////////////////////////////////////////

  //TODO: perhaps save the cursor position when done in a global struct
  // TODO: wraparound when reach the bottom / right edge

////////////////////////////////////////////////////

//IDEA: Once get video output for printing, check if we're in long mode
// Would be great if std libs could be used, too. Wouldn't need much more than Printf and getchar() the way we're doing things, though.
// TODO: keyboard driver (PS/2 for starters, then USB)

/*
Note:

typedef enum {
  EfiResetCold, // Power cycle (hard off/on)
  EfiResetWarm, //
  EfiResetShutdown // Uh, normal shutdown.
} EFI_RESET_TYPE;

typedef
VOID
ResetSystem (IN EFI_RESET_TYPE ResetType, IN EFI_STATUS ResetStatus, IN UINTN DataSize, IN VOID *ResetData OPTIONAL);

RT->ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL); // Shutdown the system
RT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL); // Hard reboot the system
RT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL); // Soft reboot the system

There is also EfiResetPlatformSpecific, but that's not really important. (Takes a standard 128-bit EFI_GUID as ResetData for a custom reset type)
*/
