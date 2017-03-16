/* mbed simplified access to Microchip 24LCxx Serial EEPROM devices (I2C)
 * Copyright (c) 2010-2012 ygarcia, MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
 * and associated documentation files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, 
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or 
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <iostream>
#include <sstream>

#include "24LCxx_I2C.h"

namespace _24LCXX_I2C {

    unsigned char C24LCXX_I2C::I2CModuleRefCounter = 0;

    C24LCXX_I2C::C24LCXX_I2C(const PinName p_sda, const PinName p_scl, const unsigned char p_address, const PinName p_wp, const unsigned int p_frequency) : _internalId("") {
        if (C24LCXX_I2C::I2CModuleRefCounter != 0) {
            error("C24LCXX_I2C: Wrong params");
        }

        _i2cInstance = new I2C(p_sda, p_scl);
        C24LCXX_I2C::I2CModuleRefCounter += 1;

        _slaveAddress = (p_address << 1) | 0xa0; // Slave address format is: 1 0 1 0 A3 A2 A1 R/W
        _i2cInstance->frequency(p_frequency); // Set the frequency of the I2C interface
        if (p_wp != NC) {
            _wp = new DigitalOut(p_wp);
            _wp->write(0); // Disable write protect
        } else {
            _wp = NULL; // Not used
        }
    
    }

    C24LCXX_I2C::~C24LCXX_I2C() {
    
        // Release I2C instance
        C24LCXX_I2C::I2CModuleRefCounter -= 1;
        if (C24LCXX_I2C::I2CModuleRefCounter == 0) {
            delete _i2cInstance;
            _i2cInstance = NULL;
        }
        // Release _wp if required
        if (_wp != NULL) {
            _wp->write(0);
            delete _wp;
        }
    
    }
    
    bool C24LCXX_I2C::WriteProtect(const bool p_writeProtect) {
        if (_wp != NULL) {
            _wp->write((int)(p_writeProtect));
            return true;
        }
    
        return false;
    }

    bool C24LCXX_I2C::EraseMemoryArea(const short p_startAddress, const int p_count, const unsigned char p_pattern) {
    
        std::vector<unsigned char> eraseBuffer(p_count, p_pattern);
        return Write(p_startAddress, eraseBuffer, false);
    }

    bool C24LCXX_I2C::Write(const short p_address, const unsigned char p_byte) {
    
        // 1.Prepare buffer
        char i2cBuffer[3]; // Memory address + one byte of data
        // 1.1. Memory address
        short address = p_address + 1; // Index start to 1
        i2cBuffer[0] = (unsigned char)(address >> 8);
        i2cBuffer[1] = (unsigned char)((unsigned char)address & 0xff);
        // 1.2. Datas
        i2cBuffer[2] = p_byte;
    
        // 2. Send I2C start + I2C address + Memory Address + Datas + I2C stop
        int result = _i2cInstance->write(_slaveAddress, i2cBuffer, 3);
        wait(0.02);
    
        return (bool)(result == 0);
    }
    
    bool C24LCXX_I2C::Write(const short p_address, const short p_short, const C24LCXX_I2C::Mode p_mode) {
    
        // 1.Prepare buffer
        char i2cBuffer[4]; // Memory address + one short (2 bytes)
        // 1.1. Memory address
        short address = p_address + 1; // Index start to 1
        i2cBuffer[0] = (unsigned char)(address >> 8);
        i2cBuffer[1] = (unsigned char)((unsigned char)address & 0xff);
        // 1.2. Datas
        if (p_mode == BigEndian) {
            i2cBuffer[2] = (unsigned char)(p_short >> 8);
            i2cBuffer[3] = (unsigned char)((unsigned char)p_short & 0xff);
        } else {
            i2cBuffer[2] = (unsigned char)((unsigned char)p_short & 0xff);
            i2cBuffer[3] = (unsigned char)(p_short >> 8);
        }
    
        // 2. Send I2C start + I2C address + Memory Address + Datas + I2C stop
        int result = _i2cInstance->write(_slaveAddress, i2cBuffer, 4);
        wait(0.02);
    
        return (bool)(result == 0);
    }
    
    bool C24LCXX_I2C::Write(const short p_address, const int p_int, const C24LCXX_I2C::Mode p_mode) {
    
        // 1.Prepare buffer
        char i2cBuffer[6]; // Memory address + one integer (4 bytes)
        // 1.1. Memory address
        short address = p_address + 1; // Index start to 1
        i2cBuffer[0] = (unsigned char)(address >> 8);
        i2cBuffer[1] = (unsigned char)((unsigned char)address & 0xff);
        // 1.2. Datas
        if (p_mode == BigEndian) {
            i2cBuffer[2] = (unsigned char)(p_int >> 24);
            i2cBuffer[3] = (unsigned char)(p_int >> 16);
            i2cBuffer[4] = (unsigned char)(p_int >> 8);
            i2cBuffer[5] = (unsigned char)((unsigned char)p_int & 0xff);
        } else {
            i2cBuffer[2] = (unsigned char)((unsigned char)p_int & 0xff);
            i2cBuffer[3] = (unsigned char)(p_int >> 8);
            i2cBuffer[4] = (unsigned char)(p_int >> 16);
            i2cBuffer[5] = (unsigned char)(p_int >> 24);
        }
    
        // 2. Send I2C start + I2C address + Memory Address + Datas + I2C stop
        int result = _i2cInstance->write(_slaveAddress, i2cBuffer, 6);
        wait(0.02);
    
        return (bool)(result == 0);
    }
    
    bool C24LCXX_I2C::Write(const short p_address, const std::string & p_string, const bool p_storeLength, const int p_length2write) {
        return Write(p_address, p_string.c_str(), p_storeLength, p_length2write);
    }
    
    bool C24LCXX_I2C::Write(const short p_address, const std::vector<unsigned char> & p_datas, const bool p_storeLength, const int p_length2write) {
    
        int length = (p_length2write == -1) ? p_datas.size() : p_length2write;
        unsigned char array[length];
        std::copy(p_datas.begin(), p_datas.end(), array);
        bool result = Write(p_address, array, p_storeLength, length);
        wait(0.02);
    
        return result;
    }
    
    bool C24LCXX_I2C::Write(const short p_address, const char *p_datas, const bool p_storeLength, const int p_length2write) {
    
        // 1.Prepare buffer
        int length = (p_length2write == -1) ? strlen(p_datas) : p_length2write;
        if (p_storeLength) {
            length += 4; // Add four bytes for the length as integer
        }
        
        char i2cBuffer[2 + length];
        // 1.1. Memory address
        short address = p_address + 1;
        i2cBuffer[0] = (unsigned char)(address >> 8);
        i2cBuffer[1] = (unsigned char)((unsigned char)address & 0xff);
        // 1.2. Datas
        if (p_storeLength) {
            // Fill the length
            i2cBuffer[2] = (unsigned char)(length >> 24);
            i2cBuffer[3] = (unsigned char)(length >> 16);
            i2cBuffer[4] = (unsigned char)(length >> 8);
            i2cBuffer[5] = (unsigned char)((unsigned char)length & 0xff);
            for (int i = 0; i < length - 4; i++) {
                i2cBuffer[6 + i] = *(p_datas + i);
            }
        } else { // The length was not stored
            for (int i = 0; i < length; i++) {
                i2cBuffer[2 + i] = *(p_datas + i);
            }
        }
        
        // 2. Send I2C start + I2C address + Memory Address + Datas + I2C stop
        int result = _i2cInstance->write(_slaveAddress, i2cBuffer, 2 + length);
        wait(0.02);
    
        return (bool)(result == 0);
    }
    
    bool C24LCXX_I2C::Write(const short p_address, const unsigned char *p_datas, const bool p_storeLength, const int p_length2write) {
        return Write(p_address, (const char *)p_datas, p_storeLength, p_length2write);
    }
    
    bool C24LCXX_I2C::Read(const short p_address, unsigned char * p_byte) {
    
        // 1.Prepare buffer
        char i2cBuffer[2];
        // 1.1. Memory address
        i2cBuffer[0] = (unsigned char)(p_address >> 8);
        i2cBuffer[1] = (unsigned char)((unsigned char)p_address & 0xff);
    
        // 2. Send I2C start + memory address
        if (_i2cInstance->write(_slaveAddress, i2cBuffer, 2, true) == 0) {
            wait(0.02);
            // 2. Read data + I2C stop
            int result = _i2cInstance->read(_slaveAddress, (char *)p_byte, 1);
            wait(0.02);
    
            return (bool)(result == 0);
        }
    
        return false;
    }
    
    bool C24LCXX_I2C::Read(const short p_address, short *p_short, const C24LCXX_I2C::Mode p_mode) {
    
        // 1.Prepare buffer
        char i2cBuffer[2];
        // 1.1. Memory address
        i2cBuffer[0] = (unsigned char)(p_address >> 8);
        i2cBuffer[1] = (unsigned char)((unsigned char)p_address & 0xff);
    
        // 2. Send I2C start + memory address
        if (_i2cInstance->write(_slaveAddress, i2cBuffer, 2, true) == 0) {
            wait(0.02);
            // 2. Read data + I2C stop
            int result = _i2cInstance->read(_slaveAddress, i2cBuffer, 2);
            if (result == 0) {
                if (p_mode ==  BigEndian) {
                    *p_short = (short)(i2cBuffer[0] << 8 | i2cBuffer[1]);
                } else {
                    *p_short = (short)(i2cBuffer[1] << 8 | i2cBuffer[0]);
                }
        
                return true;
            }
        }
    
        return false;
    }
    
    bool C24LCXX_I2C::Read(const short p_address, int *p_int, const C24LCXX_I2C::Mode p_mode) {
    
        // 1.Prepare buffer
        char i2cBuffer[4];
        // 1.1. Memory address
        i2cBuffer[0] = (unsigned char)(p_address >> 8);
        i2cBuffer[1] = (unsigned char)((unsigned char)p_address & 0xff);
    
        // 2. Send I2C start + memory address
        if (_i2cInstance->write(_slaveAddress, i2cBuffer, 2, true) == 0) {
            wait(0.02);
            // 2. Read data + I2C stop
            int result = _i2cInstance->read(_slaveAddress, i2cBuffer, 4);
            if (result == 0) {
                wait(0.02);
                if (p_mode ==  BigEndian) {
                    *p_int = (int)(i2cBuffer[0] << 24 | i2cBuffer[1] << 16 | i2cBuffer[2] << 8 | i2cBuffer[3]);
                } else {
                    *p_int = (int)(i2cBuffer[3] << 24 | i2cBuffer[2] << 16 | i2cBuffer[1] << 8 | i2cBuffer[0]);
                }
    
                return true;
            }
    
            return false;
        }
    
        return false;
    }
    
    bool C24LCXX_I2C::Read(const short p_address, std::vector<unsigned char> & p_datas, const bool p_readLengthFirst, const int p_length2write) {
    
        // 1.Prepare buffer
        short address = p_address;
        int length = 0;
        if (p_readLengthFirst) {
            if (!Read(address, &length)) { // Read the length in big endian mode
                return false;
            }
            if (length == 0) {
                return true;
            }
            address += 4; // Skip the length value 
            length -= 4; // length is the size of (string length + string)
        } else {
            if (p_length2write == -1) {
                length = p_datas.size();
            } else {
                length = p_length2write;
            }
        }
    
        // 2. Memory address
        char i2cBuffer[2];
        i2cBuffer[0] = (unsigned char)(address >> 8);
        i2cBuffer[1] = (unsigned char)((unsigned char)address & 0xff);
    
        // 3. Send I2C start + memory address
        if (_i2cInstance->write(_slaveAddress, i2cBuffer, 2, true) == 0) {
            wait(0.02);
            // 4. read data + I2C stop
            unsigned char buffer[length];
            int result = _i2cInstance->read(_slaveAddress, (char *)buffer, length);
            wait(0.02);
            if (result == 0) {
                p_datas.assign(buffer, buffer + length);
    
                return (bool)(result == 0);
            }
        }
    
        return false;
    }
    
    bool C24LCXX_I2C::Read(const short p_address, std::string & p_string, const bool p_readLengthFirst, const int p_length2write) {

/*        std::vector<unsigned char> datas;
        if (Read(p_address, datas, p_readLengthFirst, p_length2write) == true) {
            p_string.assign((char *)datas.begin(), datas.size());
            
            return true;
        }
        
        DEBUG_LEAVE("C24LCXX_I2C::Read (string) (false)")
        return false;
*/        
    
        // 1.Prepare buffer
        short address = p_address;
        int length = -1;
        if (p_readLengthFirst) { // The string was stored with its length
            if (!Read(address, &length)) { // Read the length as integer in big endian mode
                return false;
            }
            wait(0.02);
            if (length == 0) {
                return true;
            }
            address += 4; // Skip the length value size 
            length -= 4; // length is the size of (string length + string)
        } else { // The string length is provided by p_length2write parameter
            if (p_length2write == -1) {
                length = p_string.size();
            } else {
                length = p_length2write;
                p_string.resize(p_length2write);
            }
        }
    
        // 2. Memory address
        char i2cBuffer[2];
        i2cBuffer[0] = (unsigned char)(address >> 8);
        i2cBuffer[1] = (unsigned char)((unsigned char)address & 0xff);
    
        // 3. Send I2C start + memory address with repeat start
        if (_i2cInstance->write(_slaveAddress, i2cBuffer, 2, true) == 0) {
            wait(0.02);
            // 4. Read data + I2C stop
            char buffer[length];
            int result = _i2cInstance->read(_slaveAddress, (char *)buffer, length);
            if (result == 0) {
                p_string.assign(buffer, length);
        
                return true;
            }
        }
        
        return false;
    }

} // End of namespace _24LCXX_I2C
