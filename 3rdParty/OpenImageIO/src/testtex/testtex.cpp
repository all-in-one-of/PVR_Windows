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


#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <iostream>
#include <iterator>

#include <Imath/ImathMatrix.h>
#include <Imath/ImathVec.h>

#include "argparse.h"
#include "imageio.h"
#include "ustring.h"
#include "imagebuf.h"
#include "imagebufalgo.h"
#include "texture.h"
#include "fmath.h"
#include "sysutil.h"
#include "strutil.h"
#include "timer.h"
#include "../libtexture/imagecache_pvt.h"

OIIO_NAMESPACE_USING

static std::vector<std::string> filenames;
static std::string output_filename = "out.exr";
static bool verbose = false;
static int output_xres = 512, output_yres = 512;
static std::string dataformatname = "half";
static float sscale = 1, tscale = 1;
static float sblur = 0, tblur = -1;
static float width = 1;
static std::string wrapmodes ("periodic");
static int anisotropic = -1;
static int iters = 1;
static int autotile = 0;
static bool automip = false;
static bool test_construction = false;
static bool test_gettexels = false;
static bool test_getimagespec = false;
static bool filtertest = false;
static TextureSystem *texsys = NULL;
static std::string searchpath;
static int blocksize = 1;
static bool nowarp = false;
static bool tube = false;
static bool use_handle = false;
static float cachesize = -1;
static int maxfiles = -1;
static float missing[4] = {-1, 0, 0, 1};
static float fill = -1;  // -1 signifies unset
static float scalefactor = 1.0f;
static Imath::V3f offset (0,0,0);
static bool nountiled = false;
static bool nounmipped = false;
static bool gray_to_rgb = false;
static bool resetstats = false;
static bool testhash = false;
static Imath::M33f xform;
void *dummyptr;



static int
parse_files (int argc, const char *argv[])
{
    for (int i = 0;  i < argc;  i++)
        filenames.push_back (argv[i]);
    return 0;
}



static void
getargs (int argc, const char *argv[])
{
    TextureOptions opt;  // to figure out defaults
    anisotropic = opt.anisotropic;

    bool help = false;
    ArgParse ap;
    ap.options ("Usage:  testtex [options] inputfile",
                  "%*", parse_files, "",
                  "--help", &help, "Print help message",
                  "-v", &verbose, "Verbose status messages",
                  "-o %s", &output_filename, "Output test image",
                  "-d %s", &dataformatname, "Set the output data format to one of:"
                        "uint8, sint8, uint10, uint12, uint16, sint16, half, float, double",
                  "-res %d %d", &output_xres, &output_yres,
                      "Resolution of output test image",
                  "-iters %d", &iters,
                      "Iterations for time trials",
                  "--blur %f", &sblur, "Add blur to texture lookup",
                  "--stblur %f %f", &sblur, &tblur, "Add blur (s, t) to texture lookup",
                  "--width %f", &width, "Multiply filter width of texture lookup",
                  "--fill %f", &fill, "Set fill value for missing channels",
                  "--wrap %s", &wrapmodes, "Set wrap mode (default, black, clamp, periodic, mirror, overscan)",
                  "--aniso %d", &anisotropic,
                      Strutil::format("Set max anisotropy (default: %d)", anisotropic).c_str(),
                  "--missing %f %f %f", &missing[0], &missing[1], &missing[2],
                        "Specify missing texture color",
                  "--autotile %d", &autotile, "Set auto-tile size for the image cache",
                  "--automip", &automip, "Set auto-MIPmap for the image cache",
                  "--blocksize %d", &blocksize, "Set blocksize (n x n) for batches",
                  "--handle", &use_handle, "Use texture handle rather than name lookup",
                  "--searchpath %s", &searchpath, "Search path for files",
                  "--filtertest", &filtertest, "Test the filter sizes",
                  "--nowarp", &nowarp, "Do not warp the image->texture mapping",
                  "--tube", &tube, "Make a tube projection",
                  "--ctr", &test_construction, "Test TextureOpt construction time",
                  "--gettexels", &test_gettexels, "Test TextureSystem::get_texels",
                  "--getimagespec", &test_getimagespec, "Test TextureSystem::get_imagespec",
                  "--offset %f %f %f", &offset[0], &offset[1], &offset[2], "Offset texture coordinates",
                  "--scalest %f %f", &sscale, &tscale, "Scale texture lookups (s, t)",
                  "--cachesize %f", &cachesize, "Set cache size, in MB",
                  "--scale %f", &scalefactor, "Scale intensities",
                  "--maxfiles %d", &maxfiles, "Set maximum open files",
                  "--nountiled", &nountiled, "Reject untiled images",
                  "--nounmipped", &nounmipped, "Reject unmipped images",
                  "--graytorgb", &gray_to_rgb, "Convert gratscale textures to RGB",
                  "--resetstats", &resetstats, "Print and reset statistics on each iteration",
                  "--testhash", &testhash, "Test the tile hashing function",
                  NULL);
    if (ap.parse (argc, argv) < 0) {
        std::cerr << ap.geterror() << std::endl;
        ap.usage ();
        exit (EXIT_FAILURE);
    }
    if (help) {
        ap.usage ();
        exit (EXIT_FAILURE);
    }

    if (filenames.size() < 1 &&
          !test_construction && !test_getimagespec && !testhash) {
        std::cerr << "testtex: Must have at least one input file\n";
        ap.usage();
        exit (EXIT_FAILURE);
    }
}



static void
test_gettextureinfo (ustring filename)
{
    bool ok;

    int res[2] = {0};
    ok = texsys->get_texture_info (filename, 0, ustring("resolution"),
                                   TypeDesc(TypeDesc::INT,2), res);
    std::cerr << "Result of get_texture_info resolution = " << ok << ' ' << res[0] << 'x' << res[1] << "\n";

    int chan = 0;
    ok = texsys->get_texture_info (filename, 0, ustring("channels"),
                                   TypeDesc::INT, &chan);
    std::cerr << "Result of get_texture_info channels = " << ok << ' ' << chan << "\n";

    float fchan = 0;
    ok = texsys->get_texture_info (filename, 0, ustring("channels"),
                                   TypeDesc::FLOAT, &fchan);
    std::cerr << "Result of get_texture_info channels = " << ok << ' ' << fchan << "\n";

    int dataformat = 0;
    ok = texsys->get_texture_info (filename, 0, ustring("format"),
                                   TypeDesc::INT, &dataformat);
    std::cerr << "Result of get_texture_info data format = " << ok << ' ' 
              << TypeDesc((TypeDesc::BASETYPE)dataformat).c_str() << "\n";

    const char *datetime = NULL;
    ok = texsys->get_texture_info (filename, 0, ustring("DateTime"),
                                   TypeDesc::STRING, &datetime);
    std::cerr << "Result of get_texture_info datetime = " << ok << ' ' 
              << (datetime ? datetime : "") << "\n";

    const char *texturetype = NULL;
    ok = texsys->get_texture_info (filename, 0, ustring("textureformat"),
                                   TypeDesc::STRING, &texturetype);
    std::cerr << "Texture type is " << ok << ' '
              << (texturetype ? texturetype : "") << "\n";
    std::cerr << "\n";
}



static void
adjust_spec (ImageSpec &outspec, const std::string &dataformatname)
{
    if (! dataformatname.empty()) {
        if (dataformatname == "uint8")
            outspec.set_format (TypeDesc::UINT8);
        else if (dataformatname == "int8")
            outspec.set_format (TypeDesc::INT8);
        else if (dataformatname == "uint10") {
            outspec.attribute ("oiio:BitsPerSample", 10);
            outspec.set_format (TypeDesc::UINT16);
        }
        else if (dataformatname == "uint12") {
            outspec.attribute ("oiio:BitsPerSample", 12);
            outspec.set_format (TypeDesc::UINT16);
        }
        else if (dataformatname == "uint16")
            outspec.set_format (TypeDesc::UINT16);
        else if (dataformatname == "int16")
            outspec.set_format (TypeDesc::INT16);
        else if (dataformatname == "half")
            outspec.set_format (TypeDesc::HALF);
        else if (dataformatname == "float")
            outspec.set_format (TypeDesc::FLOAT);
        else if (dataformatname == "double")
            outspec.set_format (TypeDesc::DOUBLE);
        outspec.channelformats.clear ();
    }
}



inline Imath::V3f
warp (float x, float y, Imath::M33f &xform)
{
    Imath::V3f coord (x, y, 1.0f);
    coord *= xform;
    coord[0] *= 1/(1+2*std::max (-0.5f, coord[1]));
    return coord;
}


inline Imath::V3f
warp (float x, float y, float z, Imath::M33f &xform)
{
    Imath::V3f coord (x, y, z);
    coord *= xform;
    coord[0] *= 1/(1+2*std::max (-0.5f, coord[1]));
    return coord;
}



// Just map pixels to [0,1] st space
static void
map_default (int x, int y, float &s, float &t,
             float &dsdx, float &dtdx, float &dsdy, float &dtdy)
{
    s = float(x+0.5f)/output_xres * sscale + offset[0];
    t = float(y+0.5f)/output_yres * tscale + offset[1];
    dsdx = 1.0f/output_xres * sscale;
    dtdx = 0.0f;
    dsdy = 0.0f;
    dtdy = 1.0f/output_yres * tscale;
}



static void
map_warp (int x, int y, float &s, float &t,
          float &dsdx, float &dtdx, float &dsdy, float &dtdy)
{
    Imath::V3f coord = warp (float(x+0.5f)/output_xres, float(y+0.5f)/output_yres, xform);
    coord.x *= sscale;
    coord.y *= tscale;
    coord += offset;
    Imath::V3f coordx = warp (float(x+1.5f)/output_xres, float(y+0.5f)/output_yres, xform);
    coordx.x *= sscale;
    coordx.y *= tscale;
    coordx += offset;
    Imath::V3f coordy = warp (float(x+0.5f)/output_xres, float(y+1.5f)/output_yres, xform);
    coordy.x *= sscale;
    coordy.y *= tscale;
    coordy += offset;
    s = coord[0];
    t = coord[1];
    dsdx = coordx[0] - coord[0];
    dtdx = coordx[1] - coord[1];
    dsdy = coordy[0] - coord[0];
    dtdy = coordy[1] - coord[1];
}



static void
map_tube (int x, int y, float &s, float &t,
          float &dsdx, float &dtdx, float &dsdy, float &dtdy)
{
    float xt = float(x+0.5f)/output_xres - 0.5f;
    float dxt_dx = 1.0f/output_xres;
    float yt = float(y+0.5f)/output_yres - 0.5f;
    float dyt_dy = 1.0f/output_yres;
    float theta = atan2f (yt, xt);
    // See OSL's Dual2 for partial derivs of
    // atan2, hypot, and 1/x
    float denom = 1.0f / (xt*xt + yt*yt);
    float dtheta_dx = yt*dxt_dx * denom;
    float dtheta_dy = -xt*dyt_dy * denom;
    s = 4.0f * theta / (2.0f * M_PI);
    dsdx = 4.0f * dtheta_dx / (2.0f * M_PI);
    dsdy = 4.0f * dtheta_dy / (2.0f * M_PI);
    float h = hypot(xt,yt);
    float dh_dx = xt*dxt_dx / h;
    float dh_dy = yt*dyt_dy / h;
    h *= M_SQRT2;
    dh_dx *= M_SQRT2; dh_dy *= M_SQRT2;
    float hinv = 1.0f / h;
    t = hinv;
    dtdx = hinv * (-hinv * dh_dx);
    dtdy = hinv * (-hinv * dh_dy);
}



// To test filters, we always sample at the center of the image, and
// keep the minor axis of the filter at 1/256, but we vary the
// eccentricity (i.e. major axis length) as we go left (1) to right
// (32), and vary the angle as we go top (0) to bottom (2pi).
//
// If filtering is correct, all pixels should sample from the same MIP
// level because they have the same minor axis (1/256), regardless of
// eccentricity or angle.  If we specify a texture that has a
// distinctive color at the 256-res level, and something totally
// different at the 512 and 128 levels, it should be easy to verify that
// we aren't over-filtering or under-filtering by selecting the wrong
// MIP level.  (Though of course, there are other kinds of mistakes we
// could be making, such as computing the wrong eccentricity or angle.)
static void
map_filtertest (int x, int y, float &s, float &t,
                float &dsdx, float &dtdx, float &dsdy, float &dtdy)
{
    float minoraxis = 1.0f/256;
    float majoraxis = minoraxis * lerp (1.0f, 32.0f, (float)x/(output_xres-1));
    float angle = 2.0f * M_PI * (float)y/(output_yres-1);
    float sinangle, cosangle;
    sincos (angle, &sinangle, &cosangle);
    s = 0.5f;
    t = 0.5f;

    dsdx =  minoraxis * cosangle;
    dtdx =  minoraxis * sinangle;
    dsdy = -majoraxis * sinangle;
    dtdy =  majoraxis * cosangle;
}



void
map_default_3D (int x, int y, Imath::V3f &P,
                Imath::V3f &dPdx, Imath::V3f &dPdy, Imath::V3f &dPdz)
{
    P[0] = (float)(x+0.5f)/output_xres * sscale;
    P[1] = (float)(y+0.5f)/output_yres * tscale;
    P[2] = 0.5f * sscale;
    P += offset;
    dPdx[0] = 1.0f/output_xres * sscale;
    dPdx[1] = 0;
    dPdx[2] = 0;
    dPdy[0] = 0;
    dPdy[1] = 1.0f/output_yres * tscale;
    dPdy[2] = 0;
    dPdz.setValue (0,0,0);
}



void
map_warp_3D (int x, int y, Imath::V3f &P,
             Imath::V3f &dPdx, Imath::V3f &dPdy, Imath::V3f &dPdz)
{
    Imath::V3f coord = warp ((float)x/output_xres,
                             (float)y/output_yres,
                             0.5, xform);
    coord.x *= sscale;
    coord.y *= tscale;
    coord += offset;
    Imath::V3f coordx = warp ((float)(x+1)/output_xres,
                              (float)y/output_yres,
                              0.5, xform);
    coordx.x *= sscale;
    coordx.y *= tscale;
    coordx += offset;
    Imath::V3f coordy = warp ((float)x/output_xres,
                              (float)(y+1)/output_yres,
                              0.5, xform);
    coordy.x *= sscale;
    coordy.y *= tscale;
    coordy += offset;
    P = coord;
    dPdx = coordx - coord;
    dPdy = coordy - coord;
    dPdz.setValue (0,0,0);
}



template<class MAPPING>
void
test_plain_texture (MAPPING mapping)
{
    std::cerr << "Testing 2d texture " << filenames[0] << ", output = " 
              << output_filename << "\n";
    const int nchannels = 4;
    ImageSpec outspec (output_xres, output_yres, nchannels, TypeDesc::HALF);
    adjust_spec (outspec, dataformatname);
    ImageBuf image (output_filename, outspec);
    ImageBufAlgo::zero (image);

    Imath::M33f scale;  scale.scale (Imath::V2f (0.5, 0.5));
    Imath::M33f rot;    rot.rotate (radians(30.0f));
    Imath::M33f trans;  trans.translate (Imath::V2f (0.35f, 0.15f));
    xform = scale * rot * trans;
    xform.invert();

    TextureOptions opt;
    opt.sblur = sblur;
    opt.tblur = tblur >= 0.0f ? tblur : sblur;
    opt.swidth = width;
    opt.twidth = width;
    opt.nchannels = nchannels;
    
    float localfill = (fill >= 0.0f) ? fill : 1.0f;
    opt.fill = localfill;
    if (missing[0] >= 0)
        opt.missingcolor.init ((float *)&missing, 0);
//    opt.interpmode = TextureOptions::InterpSmartBicubic;
//    opt.mipmode = TextureOptions::MipModeAniso;
    TextureOptions::parse_wrapmodes (wrapmodes.c_str(), opt.swrap, opt.twrap);
    opt.anisotropic = anisotropic;

    TextureOpt opt1;
    opt1.sblur = sblur;
    opt1.tblur = tblur >= 0.0f ? tblur : sblur;
    opt1.swidth = width;
    opt1.twidth = width;
    opt1.nchannels = nchannels;
    opt1.fill = localfill;
    if (missing[0] >= 0)
        opt1.missingcolor = (float *)&missing;
    TextureOpt::parse_wrapmodes (wrapmodes.c_str(), opt1.swrap, opt1.twrap);
    opt1.anisotropic = anisotropic;

    int shadepoints = blocksize*blocksize;
    float *s = ALLOCA (float, shadepoints);
    float *t = ALLOCA (float, shadepoints);
    Runflag *runflags = ALLOCA (Runflag, shadepoints);
    float *dsdx = ALLOCA (float, shadepoints);
    float *dtdx = ALLOCA (float, shadepoints);
    float *dsdy = ALLOCA (float, shadepoints);
    float *dtdy = ALLOCA (float, shadepoints);
    float *result = ALLOCA (float, shadepoints*nchannels);
    
    ustring filename = ustring (filenames[0]);
    TextureSystem::Perthread *perthread_info = texsys->get_perthread_info ();
    TextureSystem::TextureHandle *texture_handle = texsys->get_texture_handle (filename);

    for (int iter = 0;  iter < iters;  ++iter) {
        if (iters > 1 && filenames.size() > 1) {
            // Use a different filename for each iteration
            int texid = std::min (iter, (int)filenames.size()-1);
            filename = ustring (filenames[texid]);
            std::cerr << "iter " << iter << " file " << filename << "\n";
        }

        // Iterate over blocks
        for (int by = 0, b = 0;  by < output_yres;  by+=blocksize) {
            for (int bx = 0;  bx < output_xres;  bx+=blocksize, ++b) {
                // Trick: switch to other textures on later iterations, if any
                if (iters == 1 && filenames.size() > 1) {
                    // Use a different filename from block to block
                    filename = ustring (filenames[b % (int)filenames.size()]);
                }

                // Process pixels within a block.  First save the texture warp
                // (s,t) and derivatives into SIMD vectors.
                int idx = 0;
                for (int y = by; y < by+blocksize; ++y) {
                    for (int x = bx; x < bx+blocksize; ++x) {
                        if (x < output_xres && y < output_yres) {
                            mapping (x, y, s[idx], t[idx],
                                     dsdx[idx], dtdx[idx], dsdy[idx], dtdy[idx]);
                            runflags[idx] = RunFlagOn;
                        } else {
                            runflags[idx] = RunFlagOff;
                        }
                        ++idx;
                    }
                }

                // Call the texture system to do the filtering.
                bool ok;
                if (blocksize == 1) {
                    if (use_handle)
                        ok = texsys->texture (texture_handle, perthread_info, opt1,
                                              s[0], t[0], dsdx[0], dtdx[0],
                                              dsdy[0], dtdy[0], result);
                    else
                        ok = texsys->texture (filename, opt1,
                                              s[0], t[0], dsdx[0], dtdx[0],
                                              dsdy[0], dtdy[0], result);
                } else {
                    ok = texsys->texture (filename, opt, runflags, 0,
                                          shadepoints, Varying(s), Varying(t),
                                          Varying(dsdx), Varying(dtdx),
                                          Varying(dsdy), Varying(dtdy), result);
                }
                if (! ok) {
                    std::string e = texsys->geterror ();
                    if (! e.empty())
                        std::cerr << "ERROR: " << e << "\n";
                }
                for (int i = 0;  i < shadepoints*nchannels;  ++i)
                    result[i] *= scalefactor;

                // Save filtered pixels back to the image.
                idx = 0;
                for (int y = by; y < by+blocksize; ++y) {
                    for (int x = bx; x < bx+blocksize; ++x) {
                        if (runflags[idx]) {
                            image.setpixel (x, y, result + idx*nchannels);
                        }
                        ++idx;
                    }
                }
            }
        }

        if (resetstats) {
            std::cout << texsys->getstats(2) << "\n";
            texsys->reset_stats ();
        }
    }
    
    if (! image.save ()) 
        std::cerr << "Error writing " << output_filename 
                  << " : " << image.geterror() << "\n";
}



template<class MAPPING>
void
test_texture3d (ustring filename, MAPPING mapping)
{
    std::cerr << "Testing 3d texture " << filename << ", output = " 
              << output_filename << "\n";
    const int nchannels = 4;
    ImageSpec outspec (output_xres, output_yres, nchannels, TypeDesc::HALF);
    adjust_spec (outspec, dataformatname);
    ImageBuf image (output_filename, outspec);
    ImageBufAlgo::zero (image);

    Imath::M33f scale;  scale.scale (Imath::V2f (0.5, 0.5));
    Imath::M33f rot;    rot.rotate (radians(30.0f));
    Imath::M33f trans;  trans.translate (Imath::V2f (0.35f, 0.15f));
    xform = scale * rot * trans;
    xform.invert();

    TextureOptions opt;
    opt.sblur = sblur;
    opt.tblur = tblur >= 0.0f ? tblur : sblur;
    opt.rblur = sblur;
    opt.swidth = width;
    opt.twidth = width;
    opt.rwidth = width;
    opt.nchannels = nchannels;
    float localfill = (fill >= 0 ? fill : 0.0f);
    opt.fill = localfill;
    if (missing[0] >= 0)
        opt.missingcolor.init ((float *)&missing, 0);

    opt.swrap = opt.twrap = opt.rwrap = TextureOptions::WrapPeriodic;
    opt.anisotropic = anisotropic;
    int shadepoints = blocksize*blocksize;
    Imath::V3f *P = ALLOCA (Imath::V3f, shadepoints);
    Runflag *runflags = ALLOCA (Runflag, shadepoints);
    Imath::V3f *dPdx = ALLOCA (Imath::V3f, shadepoints);
    Imath::V3f *dPdy = ALLOCA (Imath::V3f, shadepoints);
    Imath::V3f *dPdz = ALLOCA (Imath::V3f, shadepoints);
    float *result = ALLOCA (float, shadepoints*nchannels);
    
    for (int iter = 0;  iter < iters;  ++iter) {
        // Iterate over blocks

        // Trick: switch to second texture, if given, for second iteration
        if (iter && filenames.size() > 1)
            filename = ustring (filenames[1]);

        for (int by = 0;  by < output_yres;  by+=blocksize) {
            for (int bx = 0;  bx < output_xres;  bx+=blocksize) {
                // Process pixels within a block.  First save the texture warp
                // (s,t) and derivatives into SIMD vectors.
                int idx = 0;
                for (int y = by; y < by+blocksize; ++y) {
                    for (int x = bx; x < bx+blocksize; ++x) {
                        if (x < output_xres && y < output_yres) {
                            mapping (x, y, P[idx], dPdx[idx], dPdy[idx], dPdz[idx]);
                            runflags[idx] = RunFlagOn;
                        } else {
                            runflags[idx] = RunFlagOff;
                        }
                        ++idx;
                    }
                }
                // Call the texture system to do the filtering.
                bool ok = texsys->texture3d (filename, opt, runflags, 0, shadepoints,
                                             Varying(P), Varying(dPdx),
                                             Varying(dPdy), Varying(dPdz),
                                             result);
                if (! ok) {
                    std::string e = texsys->geterror ();
                    if (! e.empty())
                        std::cerr << "ERROR: " << e << "\n";
                }
                for (int i = 0;  i < shadepoints*nchannels;  ++i)
                    result[i] *= scalefactor;

                // Save filtered pixels back to the image.
                idx = 0;
                for (int y = by; y < by+blocksize; ++y) {
                    for (int x = bx; x < bx+blocksize; ++x) {
                        if (runflags[idx]) {
                            image.setpixel (x, y, result + idx*nchannels);
                        }
                        ++idx;
                    }
                }
            }
        }
    }
    
    if (! image.save ()) 
        std::cerr << "Error writing " << output_filename 
                  << " : " << image.geterror() << "\n";
}



static void
test_shadow (ustring filename)
{
}



static void
test_environment (ustring filename)
{
}



static void
test_getimagespec_gettexels (ustring filename)
{
    ImageSpec spec;
    int miplevel = 0;
    if (! texsys->get_imagespec (filename, 0, spec)) {
        std::cerr << "Could not get spec for " << filename << "\n";
        std::string e = texsys->geterror ();
        if (! e.empty())
            std::cerr << "ERROR: " << e << "\n";
        return;
    }

    if (! test_gettexels)
        return;

    int w = spec.width / std::max(1,2<<miplevel);
    int h = spec.height / std::max(1,2<<miplevel);
    ImageSpec postagespec (w, h, spec.nchannels, TypeDesc::FLOAT);
    ImageBuf buf ("postage.exr", postagespec);
    TextureOptions opt;
    opt.nchannels = spec.nchannels;
    if (missing[0] >= 0)
        opt.missingcolor.init ((float *)&missing, 0);
    std::vector<float> tmp (w*h*spec.nchannels);
    bool ok = texsys->get_texels (filename, opt, miplevel,
                                  spec.x+w/2, spec.x+w/2+w,
                                  spec.y+h/2, spec.y+h/2+h,
                                  0, 1, postagespec.format, &tmp[0]);
    if (! ok)
        std::cerr << texsys->geterror() << "\n";
    for (int y = 0;  y < h;  ++y)
        for (int x = 0;  x < w;  ++x) {
            imagesize_t offset = (y*w + x) * spec.nchannels;
            buf.setpixel (x, y, &tmp[offset]);
        }
    buf.save ();
}



static void
test_hash ()
{
    std::vector<size_t> fourbits (1<<4, 0);
    std::vector<size_t> eightbits (1<<8, 0);
    std::vector<size_t> sixteenbits (1<<16, 0);
    std::vector<size_t> highereightbits (1<<8, 0);

    const size_t iters = 1000000;
    const int res = 4*1024;  // Simulate tiles from a 4k image
    const int tilesize = 64;
    const int nfiles = iters / ((res/tilesize)*(res/tilesize));
    std::cout << "Testing hashing with " << nfiles << " files of "
              << res << 'x' << res << " with " << tilesize << 'x' << tilesize
              << " tiles:\n";

    ImageCache *imagecache = ImageCache::create ();

    // Set up the ImageCacheFiles outside of the timing loop
    using OIIO::pvt::ImageCacheImpl;
    using OIIO::pvt::ImageCacheFile;
    using OIIO::pvt::ImageCacheFileRef;
    std::vector<ImageCacheFileRef> icf;
    for (int f = 0;  f < nfiles;  ++f) {
        ustring filename = ustring::format ("%06d.tif", f);
        icf.push_back (new ImageCacheFile(*(ImageCacheImpl *)imagecache, NULL, filename));
    }

    // First, just try to do raw timings of the hash
    Timer timer;
    size_t i = 0, hh = 0;
    for (int f = 0;  f < nfiles;  ++f) {
        for (int y = 0;  y < res;  y += tilesize) {
            for (int x = 0;  x < res;  x += tilesize, ++i) {
                OIIO::pvt::TileID id (*icf[f], 0, 0, x, y, 0);
                size_t h = id.hash();
                hh += h;
            }
        }
    }
    std::cout << "hh = " << hh << "\n";
    double time = timer();
    double rate = (i/1.0e6) / time;
    std::cout << "Hashing rate: " << Strutil::format ("%3.2f", rate)
              << " Mhashes/sec\n";

    // Now, check the quality of the hash by looking at the low 4, 8, and
    // 16 bits and making sure that they divide into hash buckets fairly
    // evenly.
    i = 0;
    for (int f = 0;  f < nfiles;  ++f) {
        for (int y = 0;  y < res;  y += tilesize) {
            for (int x = 0;  x < res;  x += tilesize, ++i) {
                OIIO::pvt::TileID id (*icf[f], 0, 0, x, y, 0);
                size_t h = id.hash();
                ++ fourbits[h & 0xf];
                ++ eightbits[h & 0xff];
                ++ highereightbits[(h>>24) & 0xff];
                ++ sixteenbits[h & 0xffff];
                // if (i < 16) std::cout << Strutil::format("%llx\n", h);
            }
        }
    }

    size_t min, max;
    min = std::numeric_limits<size_t>::max();
    max = 0;
    for (int i = 0;  i < 16;  ++i) {
        if (fourbits[i] < min) min = fourbits[i];
        if (fourbits[i] > max) max = fourbits[i];
    }
    std::cout << "4-bit hash buckets range from "
              << min << " to " << max << "\n";

    min = std::numeric_limits<size_t>::max();
    max = 0;
    for (int i = 0;  i < 256;  ++i) {
        if (eightbits[i] < min) min = eightbits[i];
        if (eightbits[i] > max) max = eightbits[i];
    }
    std::cout << "8-bit hash buckets range from "
              << min << " to " << max << "\n";

    min = std::numeric_limits<size_t>::max();
    max = 0;
    for (int i = 0;  i < 256;  ++i) {
        if (highereightbits[i] < min) min = highereightbits[i];
        if (highereightbits[i] > max) max = highereightbits[i];
    }
    std::cout << "higher 8-bit hash buckets range from "
              << min << " to " << max << "\n";

    min = std::numeric_limits<size_t>::max();
    max = 0;
    for (int i = 0;  i < (1<<16);  ++i) {
        if (sixteenbits[i] < min) min = sixteenbits[i];
        if (sixteenbits[i] > max) max = sixteenbits[i];
    }
    std::cout << "16-bit hash buckets range from "
              << min << " to " << max << "\n";

    std::cout << "\n";

    ImageCache::destroy (imagecache);
}



int
main (int argc, const char *argv[])
{
    getargs (argc, argv);

    texsys = TextureSystem::create ();
    std::cerr << "Created texture system\n";
    texsys->attribute ("statistics:level", 2);
    texsys->attribute ("autotile", autotile);
    texsys->attribute ("automip", (int)automip);
    if (cachesize >= 0)
        texsys->attribute ("max_memory_MB", cachesize);
    if (maxfiles >= 0)
        texsys->attribute ("max_open_files", maxfiles);
    if (searchpath.length())
        texsys->attribute ("searchpath", searchpath);
    if (nountiled)
        texsys->attribute ("accept_untiled", 0);
    if (nounmipped)
        texsys->attribute ("accept_unmipped", 0);
    texsys->attribute ("gray_to_rgb", gray_to_rgb);

    if (test_construction) {
        Timer t;
        for (int i = 0;  i < 1000000000;  ++i) {
            TextureOpt opt;
            dummyptr = &opt;  // This forces the optimizer to keep the loop
        }
        std::cout << "TextureOpt construction: " << t() << " ns\n";
        TextureOpt canonical, copy;
        t.reset();
        t.start();
        for (int i = 0;  i < 1000000000;  ++i) {
            memcpy (&copy, &canonical, sizeof(TextureOpt));
            dummyptr = &copy;  // This forces the optimizer to keep the loop
        }
        std::cout << "TextureOpt memcpy: " << t() << " ns\n";
    }

    if (test_getimagespec) {
        Timer t;
        ImageSpec spec;
        ustring filename (filenames[0]);
        for (int i = 0;  i < iters;  ++i) {
            texsys->get_imagespec (filename, 0, spec);
        }
        iters = 0;
    }

    if (testhash) {
        test_hash ();
    }

    if (iters > 0 && filenames.size()) {
        ustring filename (filenames[0]);
        test_gettextureinfo (filename);
        const char *texturetype = "Plain Texture";
        texsys->get_texture_info (filename, 0, ustring("texturetype"),
                                  TypeDesc::STRING, &texturetype);
        if (! strcmp (texturetype, "Plain Texture")) {
            if (nowarp)
                test_plain_texture (map_default);
            else if (tube)
                test_plain_texture (map_tube);
            else if (filtertest)
                test_plain_texture (map_filtertest);
            else
                test_plain_texture (map_warp);
        }
        if (! strcmp (texturetype, "Volume Texture")) {
            if (nowarp)
                test_texture3d (filename, map_default_3D);
            else
                test_texture3d (filename, map_warp_3D);
        }
        if (! strcmp (texturetype, "Shadow")) {
            test_shadow (filename);
        }
        if (! strcmp (texturetype, "Environment")) {
            test_environment (filename);
        }
        test_getimagespec_gettexels (filename);
    }
    
    std::cout << "Memory use: "
              << Strutil::memformat (Sysutil::memory_used(true)) << "\n";
    TextureSystem::destroy (texsys);

    std::cout << "\nustrings: " << ustring::getstats(false) << "\n\n";
    return 0;
}
