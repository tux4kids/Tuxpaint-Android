Title: Scaling and compositing

----

The `GdkPixBuf` class contains methods to scale pixbufs, to scale
pixbufs and alpha blend against an existing image, and to scale
pixbufs and alpha blend against a solid color or checkerboard.
Alpha blending a checkerboard is a common way to show an image with
an alpha channel in image-viewing and editing software.

Note that in these functions, the terms ‘alpha blending’ and ‘compositing’
are used synonymously.

Since the full-featured functions [method@GdkPixbuf.Pixbuf.scale],
[method@GdkPixbuf.Pixbuf.composite], and [`method@GdkPixbuf.Pixbuf.composite_color`]
are rather complex to use and have many arguments, two simple
convenience functions are provided, [`method@GdkPixbuf.Pixbuf.scale_simple`]
and [`method@GdkPixbuf.Pixbuf.composite_color_simple`] which create a new
pixbuf of a given size, scale an original image to fit, and then return it.

If the destination pixbuf was created from a read only source, these
operations will force a copy into a mutable buffer.

Scaling and alpha blending functions take advantage of MMX hardware
acceleration on systems where MMX is supported. If `GdkPixbuf` is built
with the Sun mediaLib library, these functions are instead accelerated
using mediaLib, which provides hardware acceleration on Intel, AMD,
and Sparc chipsets. If desired, mediaLib support can be turned off by
setting the `GDK_DISABLE_MEDIALIB` environment variable.

The alpha blending function used is:

```
Cd = Cs·As + Cd(1-As)
```

where `Cd` is the destination pixel color, `Cs` is the source pixel color,
and `As` is the source pixel alpha.

**NOTE**: It is recommended to use [Cairo][cairo] for scaling and
compositing, by using the contents of a `GdkPixbuf` pixel buffer as the
data for a Cairo image surface.

[cairo]: https://www.cairographics.org
