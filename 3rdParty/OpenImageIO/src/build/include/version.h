/*
  Copyright 2008 Larry Gritz and the other authors and contributors.
  All Rights Reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:
  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
  * Neither the name of the software's owners nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  (This is the Modified BSD License)
*/


#ifndef OPENIMAGEIO_VERSION_H
#define OPENIMAGEIO_VERSION_H


// Versioning of the OpenImageIO software

#define OIIO_NAMESPACE OpenImageIO
#define OIIO_VERSION_MAJOR 1
#define OIIO_VERSION_MINOR 2
#define OIIO_VERSION_PATCH 0
#define OIIO_VERSION_NS v1_2

#define OIIO_VERSION (10000 * OIIO_VERSION_MAJOR + \
                        100 * OIIO_VERSION_MINOR + \
                              OIIO_VERSION_PATCH)
// We also define the old name for backwards compatibility purposes.
#define OPENIMAGEIO_VERSION OIIO_VERSION

// Magic macros to make OPENIMAGEIO_VERSION_STRING that looks like "1.2.3"
#define OIIO_MAKE_VERSION_STRING2(a,b,c) #a "." #b "." #c
#define OIIO_MAKE_VERSION_STRING(a,b,c) OIIO_MAKE_VERSION_STRING2(a,b,c)
#define OIIO_VERSION_STRING \
    OIIO_MAKE_VERSION_STRING(OIIO_VERSION_MAJOR, \
                         OIIO_VERSION_MINOR, OIIO_VERSION_PATCH)
#define OIIO_INTRO_STRING "OpenImageIO " OIIO_VERSION_STRING " http://www.openimageio.org"


// Macros to use in each file to enter and exit the right name spaces.
#define OIIO_NAMESPACE_ENTER namespace OIIO_NAMESPACE { namespace OIIO_VERSION_NS
#define OIIO_NAMESPACE_EXIT using namespace OIIO_VERSION_NS; }
#define OIIO_NAMESPACE_USING using namespace OIIO_NAMESPACE;

// Establish the name spaces and make an alias 'OIIO' that gives us what
// everybody really wants.
namespace OIIO_NAMESPACE { namespace OIIO_VERSION_NS { } }
namespace OIIO = OIIO_NAMESPACE::OIIO_VERSION_NS;


/// Each imageio DSO/DLL should include this statement:
///      DLLPUBLIC int FORMAT_imageio_version = OPENIMAGEIO_PLUGIN_VERSION;
/// libOpenImageIO will check for compatibility this way.
/// This should get bumped any time we change the API in any way that
/// will make previously-compiled plugins break.
///
/// History:
/// Version 3 added supports_rectangles() and write_rectangle() to
/// ImageOutput, and added stride parameters to the ImageInput read
/// routines.
/// Version 10 represents forking from NVIDIA's open source version,
/// with which we break backwards compatibility.
/// Version 11 teased apart subimage versus miplevel specification in
/// the APIs and per-channel formats (introduced in OIIO 0.9).
/// Version 12 added read_scanlines(), write_scanlines(), read_tiles(),
///     write_tiles(), and ImageInput::supports(). (OIIO 1.0)
/// Version 13 added ImageInput::valid_file().  (OIIO 1.1)
/// Version 14 added ImageOutput::open() variety for multiple subimages.
/// Version 15 added support for "deep" images (changing ImageSpec,
///     ImageInput, ImageOutput).
/// Version 16 changed the ImageInput functions taking channel ranges
///     from firstchan,nchans to chbegin,chend.

#define OIIO_PLUGIN_VERSION 16

#define OIIO_PLUGIN_NAMESPACE_BEGIN OIIO_NAMESPACE_ENTER {
#define OIIO_PLUGIN_NAMESPACE_END } OIIO_NAMESPACE_EXIT

#ifdef EMBED_PLUGINS
#define OIIO_PLUGIN_EXPORTS_BEGIN
#define OIIO_PLUGIN_EXPORTS_END
#else
#define OIIO_PLUGIN_EXPORTS_BEGIN extern "C" {
#define OIIO_PLUGIN_EXPORTS_END }
#endif

#endif

