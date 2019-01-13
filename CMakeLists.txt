cmake_minimum_required(VERSION 3.13)
project(shrinkwrap C)
project(shrinkwrap_tests C)

set(CMAKE_C_STANDARD 99)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(-DMACOS_CLASSIC)

add_library(lshrinkwrap
        expat-2.1.0/lib/amigaconfig.h
        expat-2.1.0/lib/ascii.h
        expat-2.1.0/lib/asciitab.h
        expat-2.1.0/lib/expat.h
        expat-2.1.0/lib/expat_external.h
        expat-2.1.0/lib/iasciitab.h
        expat-2.1.0/lib/internal.h
        expat-2.1.0/lib/latin1tab.h
        expat-2.1.0/lib/macconfig.h
        expat-2.1.0/lib/nametab.h
        expat-2.1.0/lib/utf8tab.h
        expat-2.1.0/lib/winconfig.h
        expat-2.1.0/lib/xmlparse.c
        expat-2.1.0/lib/xmlrole.c
        expat-2.1.0/lib/xmlrole.h
        expat-2.1.0/lib/xmltok.c
        expat-2.1.0/lib/xmltok.h
        expat-2.1.0/lib/xmltok_impl.c
        expat-2.1.0/lib/xmltok_impl.h
        expat-2.1.0/lib/xmltok_ns.c
        lpng169/png.c
        lpng169/png.h
        lpng169/pngconf.h
        lpng169/pngdebug.h
        lpng169/pngerror.c
        lpng169/pngget.c
        lpng169/pnginfo.h
        lpng169/pnglibconf.h
        lpng169/pngmem.c
        lpng169/pngpread.c
        lpng169/pngpriv.h
        lpng169/pngread.c
        lpng169/pngrio.c
        lpng169/pngrtran.c
        lpng169/pngrutil.c
        lpng169/pngset.c
        lpng169/pngstruct.h
        lpng169/pngtrans.c
        lpng169/pngwio.c
        lpng169/pngwrite.c
        lpng169/pngwtran.c
        lpng169/pngwutil.c
        src/internal/minunit.h
        src/internal/shrinkwrap_curve_internal.h
        src/internal/shrinkwrap_internal_t.h
        src/internal/shrinkwrap_pixel_internal.h
        src/internal/shrinkwrap_triangle_internal.h
        src/array.c
        src/array.h
        src/pixel_t.h
        src/pngload.c
        src/pngload.h
        src/shrinkwrap.h
        src/shrinkwrap_curve.c
        src/shrinkwrap_html.c
        src/shrinkwrap_html.h
        src/shrinkwrap_pixel.c
        src/shrinkwrap_t.h
        src/shrinkwrap_triangle.c
        src/xmlload.c
        src/xmlload.h
        zlib-1.2.8/adler32.c
        zlib-1.2.8/compress.c
        zlib-1.2.8/crc32.c
        zlib-1.2.8/crc32.h
        zlib-1.2.8/deflate.c
        zlib-1.2.8/deflate.h
        zlib-1.2.8/gzclose.c
        zlib-1.2.8/gzguts.h
        zlib-1.2.8/gzlib.c
        zlib-1.2.8/gzread.c
        zlib-1.2.8/gzwrite.c
        zlib-1.2.8/infback.c
        zlib-1.2.8/inffast.c
        zlib-1.2.8/inffast.h
        zlib-1.2.8/inffixed.h
        zlib-1.2.8/inflate.c
        zlib-1.2.8/inflate.h
        zlib-1.2.8/inftrees.c
        zlib-1.2.8/inftrees.h
        zlib-1.2.8/trees.c
        zlib-1.2.8/trees.h
        zlib-1.2.8/uncompr.c
        zlib-1.2.8/zconf.h
        zlib-1.2.8/zlib.h
        zlib-1.2.8/zutil.c
        zlib-1.2.8/zutil.h
        )

add_executable(shrinkwrap_tests
        src/internal/shrinkwrap_internal_tests.c
        src/run_tests.c)

add_executable(shrinkwrap src/main.c)

target_link_libraries(shrinkwrap_tests lshrinkwrap)
target_link_libraries(shrinkwrap lshrinkwrap)
