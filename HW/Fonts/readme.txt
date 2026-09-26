The fnt2txf program converts bitmap fonts from the form provided
by the Bitmap Font Generator:

        http://www.angelcode.com/products/bmfont/

to the form supported by Mark Kilgard's texfont library:

        http://www.opengl.org/resources/code/rendering/mjktips/TexFont/TexFont.html

Included are 3 fonts converted from TrueType to txf: a serif font,
a sans-serif font, and a fixed-width font.

There are almost certainly byte swap issues on non-Win32 platforms.

The fonts were generated at a point size of 56 (=7*8) with antialiasing
turned up to the max (4) and inter-character spacing of 4 pixels (to
make for better mipmapping)
