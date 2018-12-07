#ifndef _VE_IO_OBJ_H
#define _VE_IO_OBJ_H

/** @file veIoObj.h
 \brief Contains an OBJ input/output class

 \author  gf
 $Revision: 2.6 $
 License notice (zlib license):

 (c) 2006 by Gerald Franz, gf@tuebingen.mpg.de

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

 */

#include "veStd.h"
#include "veMath.h"

namespace ve {
class geoNode;

//--- class ve::ioObj ---------------------------------------------
/// a class for OBJ (Alias/Wavefront model format) input/output
class ioObj {
public:
    /// loads obj files into geoNodes
    /**
         \param filename the file to be loaded,
         \return pointer to a ve::geoGroup in case of success or 0. */
    static geoNode * load(const std::string & filename);
    /// saves a model as OBJ
    static int save(const ve::geoNode & model, const std::string & filename);
};

} // namespace ve
#endif //  _VE_IO_OBJ_H
