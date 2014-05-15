#include "StdAfx.h"
#include "MrwDecoder.h"
/*
    RawSpeed - RAW file decoder.

    Copyright (C) 2009 Klaus Post

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

    http://www.klauspost.com
*/

namespace RawSpeed {

MrwDecoder::MrwDecoder(FileMap* file) :
    RawDecoder(file) {
  parseHeader();
}

MrwDecoder::~MrwDecoder(void) {
}

int MrwDecoder::isMRW(FileMap* input) {
  const uchar8* data = input->getData(0);
  return data[0] == 0x00 && data[1] == 0x4D && data[2] == 0x52 && data[3] == 0x4D;
}

#define get2BE(data,pos) ((((ushort16)(data)[pos]) << 8) | ((ushort16)(data)[pos+1]))

#define get4BE(data,pos) ((((uint32)(data)[pos]) << 24) | (((uint32)(data)[pos+1]) << 16) | \
                          (((uint32)(data)[pos+2]) << 8) | ((uint32)(data)[pos+3]))

#define get8LE(data,pos) ((((uint64)(data)[pos+7]) << 56) | (((uint64)(data)[pos+6]) << 48) | \
                          (((uint64)(data)[pos+5]) << 40) | (((uint64)(data)[pos+4]) << 32) | \
                          (((uint64)(data)[pos+3]) << 24) | (((uint64)(data)[pos+2]) << 16) | \
                          (((uint64)(data)[pos+1]) << 8) | ((uint64)(data)[pos]))
                        
static mrw_camera_t mrw_camera_table[] = {
  {"21860002", "DYNAX 5D"},
};

void MrwDecoder::parseHeader() {
  const unsigned char* data = mFile->getData(0);
  
  if (!isMRW(mFile))
    ThrowRDE("This isn't actually a MRW file, why are you calling me?");
    
  data_offset = get4BE(data,4)+8;
  
  // Let's just get all we need from the PRD block and be done with it
  raw_height = get2BE(data,24);
  raw_width = get2BE(data,26);
  packed = (data[28] == 12);
  cameraid = get8LE(data,16);
  cameraName = modelName(cameraid);
  if (!cameraName) {
    uchar8 cameracode[9] = {0};
    *((uint64 *) cameracode) = cameraid;
    ThrowRDE("MRW decoder: Unknown camera with ID %s", cameracode);
  }
}

const char* MrwDecoder::modelName(uint64 cameraid) {
  for (uint32 i=0; i<sizeof(mrw_camera_table)/sizeof(mrw_camera_table[0]); i++) { 
    if (*((uint64*) mrw_camera_table[i].code) == cameraid) {
        return mrw_camera_table[i].name;
    }
  }
  return NULL;
}

RawImage MrwDecoder::decodeRawInternal() {
  mRaw->dim = iPoint2D(raw_width, raw_height);
  mRaw->createData();

  uint32 imgsize = raw_width * raw_height * 3 / 2;

  if (!mFile->isValid(data_offset))
    ThrowRDE("MRW decoder: Data offset after EOF, file probably truncated");
  if (!mFile->isValid(data_offset+imgsize-1))
    ThrowRDE("MRW decoder: Image end after EOF, file probably truncated");

  ByteStream input(mFile->getData(data_offset), imgsize);
 
  try {
    Decode12BitRawBE(input, raw_width, raw_height);
  } catch (IOException &e) {
    mRaw->setError(e.what());
    // Let's ignore it, it may have delivered somewhat useful data.
  }

  return mRaw;
}

void MrwDecoder::checkSupportInternal(CameraMetaData *meta) {
  this->checkCameraSupported(meta, "MINOLTA", cameraName, "");
}

void MrwDecoder::decodeMetaDataInternal(CameraMetaData *meta) {
  //Default
  int iso = 0;

  setMetaData(meta, "MINOLTA", cameraName, "", iso);
}

} // namespace RawSpeed
