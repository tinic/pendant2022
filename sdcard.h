/*
Copyright 2020 Tinic Uro

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef SDCARD_H_
#define SDCARD_H_

#include "./version.h"

#include <stdint.h>
#include <tuple>

#include "ff.h"
#include "diskio.h"

class SDCard {
public:
    static SDCard &instance();
    void process();

    bool inserted() const;
    uint32_t blocks() const;

    bool readFromDataFile(uint8_t *outBuf, size_t offset, size_t size);
    bool dataFilePresent() const { return datafile_present; }

    bool newFirmwareAvailable() const {  
        return firmware_bootloaded && 
               firmware_release && 
               firmware_revision > GIT_REV_COUNT_INT; 
    }

private:

    FATFS FatFs;
    bool mounted = false;

    bool datafile_present = false;

    bool firmware_release = false;
    bool firmware_bootloaded = false;

    uint32_t firmware_revision = 0;

    void findFirmware();
    void findDataFile();

    friend DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff);
    friend DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count );

    void init();
    bool initialized = false;
};

#endif /* SDCARD_H_ */
