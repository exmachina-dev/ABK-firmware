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
#ifndef __AT24CXX_I2C_H__
#define __AT24CXX_I2C_H__

#include <string>
#include <vector>

#include "mbed.h" // Include mbed header + debug primitives. See DebugLibrary

#define DEBUG       (0);
#define DEBUG_ENTER         (0);
#define DEBUG_LEAVE         (0);

#define AT24C_PAGE_SIZE  (8)
namespace _AT24CXX_I2C {
    /** This class provides simplified I2C access to a Microchip 24LCxx Serial EEPROM device. V0.0.0.3
     *
     * Note that if the LPC1768 is powered in 3.3V and Microchip 24LCxx Serial EEPROM device could be powered at 3.3V or 5V.
     * In this case, you shall use a bi-directional level shifter for I2C-bus. Please refer to AN97055 (http://ics.nxp.com/support/documents/interface/pdf/an97055.pdf)
     * Microchip 24LCxx Serial EEPROM device reference:
     * - 24LC256: DS21203K
     * - 24LC64 : DS21189D
     *
     * Note that for I2C details, please visit http://www.datelec.fr/fiches/I2C.htm
     *
     * Note that this header file include following headers:
     * - <string>
     * - <vector>
     * - <mbed.h>
     *
     * @remark This class was validated with Tektronix TDS2014 oscilloscope in 3.3V and in mixte power mode 3.3V for mbed and 5V for the Microchip 24LCxx Serial EEPROM device
     * @author Yann Garcia (Don't hesitate to contact me: garcia.yann@gmail.com)
     */
    class AT24CXX_I2C { // TODO: Add EE Polling for write methods
        /** Reference counter used to guarentee unicity of the instance of I2C class
         */
        static unsigned char I2CModuleRefCounter;
        
        /** Device address input: A0, A1, A2 (Pins <1,3>). See DS21203K/DS21189D - Figure 5-1: Control Byte Format for address format details
         */
        unsigned char _slaveAddress;
        /** An unique instance of I2C class
         */
        I2C *_i2c_instance;
    public:
        /** Memory storage mode
         */
        enum Mode {
            LittleEndian, //<! Little Endian mode: 0xA0B70708 is stored as 08: MSB and A0 LSB
            BigEndian //<! Little Endian mode: 0xA0B70708 is stored as AO: MSB and 08 LSB
        };
    public:
        /** Constructor with write Protect command pin wired. Use it to manage the first I2C module on 3.3V or 5V network
         *
         * @param p_sda: MBed pin for SDA
         * @param p_scl: MBed pin for SCL
         * @param p_address: Device address input: A0, A1, A2 (Pins <1,3>)
         * @param p_wp: MBed pin to manage write Protect input. If NC, WP is not managed, default value is NC, not connected
         * @param p_frequency: Frequency of the I2C interface (SCL), default value is 400KHz
         * Example:
         * - If A1 and A2 pins are tired to Vdd and A0 is tired to Vss, address shall '00000110'B
         * - If A0 and A1 pins are tired to Vss and A2 is tired to Vdd, address shall '00000100'B
         */
        AT24CXX_I2C(const PinName p_sda, const PinName p_scl, const unsigned char p_address, const unsigned int p_frequency = 200000);
    
        /** Destructor
         */
        virtual ~AT24CXX_I2C();

        /** Used to return the unique instance of I2C instance
         */
        inline const I2C * operator * () { return (const I2C *)_i2c_instance; };

        /** Erase of memory area starting at the specified address, using the specified pattern to fill the memory area
         *
         * @param p_startAddress The address of the memory area (from 0 to N - 1, N is the number of cells of the memory)
         * @param p_count The size of the memory area to erase
         * @param p_pattern The pattern value to use to fill the memory area. Default vqlue: 0x00
         * @return true on success, false otherwise
         * Exemple:
         * @code
         * ...
         * myEEPROM.EraseMemoryArea(0, 1024); // Set to 0x00 the first 1Kb memory 
         * ...
         * @endcode
         */
        bool erase_memory(const short p_startAddress, const int p_count, const unsigned char p_pattern = 0x00);
    
        /** write a byte at the specified memory address
         *
         * @param p_address The memory address (from 0 to N - 1, N is the number of cells of the memory)
         * @param p_byte The byte value to save
         * @return true on success, false otherwise
         * Exemple:
         * @code
         * unsigned char value = 0xaa;
         * ...
         * myEEPROM.write(memoryAddress, value);
         * ...
         * @endcode
         */
        bool write(const short p_address, const unsigned char p_byte);
    
        /** write a short at the specified memory address according to the specified mode
         *
         * @param p_address The memory address (from 0 to N - 1, N is the number of cells of the memory)
         * @param p_short The short value to save
         * @param p_mode The storage mode. Default value: BigEndian
         * @return true on success, false otherwise
         * Exemple:
         * @code
         * short value = 0xcafe;
         * ...
         * myEEPROM.write(memoryAddress, value, LittleEndian);
         * ...
         * @endcode
         */
        bool write(const short p_address, const short p_short, const AT24CXX_I2C::Mode p_mode = BigEndian);
    
        /** write an integer at the specified memory address according to the specified mode
         *
         * @param p_address The memory address (from 0 to N - 1, N is the number of cells of the memory)
         * @param p_int The integer value to save
         * @param p_mode The storage mode. Default value: BigEndian
         * @return true on success, false otherwise
         * Exemple:
         * @code
         * int value = 0xcafedeca;
         * ...
         * myEEPROM.write(memoryAddress, value, LittleEndian);
         * ...
         * @endcode
         */
        bool write(const short p_address, const int p_int, const AT24CXX_I2C::Mode p_mode = BigEndian);
    
        /** write a buffer of bytes at the specified memory address
         *
         * @param p_address The memory address (from 0 to N - 1, N is the number of cells of the memory)
         * @param p_datas The buffer of bytes to save
         * @param p_storeLength If true, store also the length of the buffer in Big Endian mode, otherwise the length will be provided by p_length2write parameter. Default value: true.
         * @param p_length2write The number of bytes to write, -1 for all bytes. Default value: -1
         * @return true on success, false otherwise
         */
        bool write(const short p_address, const unsigned char *p_datas, const int p_length2write = -1);
    
        /** write a buffer of characters at the specified memory address (from 0 to N - 1, N is the number of cells of the memory)
         *
         * Note that the length of the buffer is not saved and the string is saved in Big Endian mode
         * @param p_address The memory address (from 0 to N - 1, N is the number of cells of the memory)
         * @param p_datas The string to save
         * @param p_storeLength If true, store also the length of the string in Big Endian mode, otherwise the length will be provided by p_length2write parameter. Default value: true.
         * @param length2write The number of character to write, -1 for all characters
         * @return true on success, false otherwise
         */
        bool write(const short p_address, const char *p_datas, const int p_length2write = -1);
    
        /** read a byte from the specified memory address
         *
         * @param p_address The memory address (from 0 to N - 1, N is the number of cells of the memory)
         * @param p_byte The byte value to read
         * @return true on success, false otherwise
         * Exemple:
         * @code
         * unsigned char value;
         * ...
         * myEEPROM.read(memoryAddress, (unsigned char *)&value);
         * ...
         * @endcode
         */
        bool read(const short p_address, unsigned char *p_value);
    
        /** read a short from the specified memory address
         *
         * @param p_address The memory address (from 0 to N - 1, N is the number of cells of the memory)
         * @param p_short The short value to read
         * @return true on success, false otherwise
         * Exemple:
         * @code
         * short value;
         * ...
         * myEEPROM.read(memoryAddress, (short *)&value);
         * ...
         * @endcode
         */
        bool read(const short p_address, short *p_short, AT24CXX_I2C::Mode p_mode = BigEndian);
    
        /** read an integer from the specified memory address
         *
         * @param p_address The memory address (from 0 to N - 1, N is the number of cells of the memory)
         * @param p_int The integer value to read
         * @return true on success, false otherwise
         * Exemple:
         * @code
         * int value;
         * ...
         * myEEPROM.read(memoryAddress, (int *)&value);
         * ...
         * @endcode
         */
        bool read(const short p_address, int *p_int, AT24CXX_I2C::Mode p_mode = BigEndian);
    
        /** read a buffer of characters from the specified memory address and store it into a string object
         *
         * Note that the size of the string object is used for the number of characters to read
         * @param p_address The memory address (from 0 to N - 1, N is the number of cells of the memory)
         * @param p_datas The buffer to fill
         * @param p_readLengthFirst If true, read the length first and p_length2write parameter is ignored, otherwise the length is provided by p_length2write parameter. Default value: true
         * @param p_length2read The number of character to read, -1 to use the size of the buffer
         * @return true on success, false otherwise
         * Exemple:
         * @code
         * std::string readtext;
         * ...
         * myEEPROM.read(memoryAddress, readtext);
         * ...
         * @endcode
         */
        bool read(const short p_address, unsigned char *p_datas, const int p_length2read = -1);

    
#if defined(__DEBUG)
        /** Dump a memory area
         * 
         * Note that this method is available only on debug mode
         * @param p_address The memory address (from 0 to N - 1, N is the number of cells of the memory)
         * @param p_count The number of bytes toi dump
         * @return true on success, false otherwise
         */
        void DumpMemoryArea(const int p_address, const int p_count);
        /** For debug purpose only
         */
        inline std::string & ToString() { return _internalId; };
#else // __DEBUG
        inline void DumpMemoryArea(const int p_address, const int p_count) {};
#endif // _DEBUG
        
    private:
        /** Internal reference identifier
         */
        std::string _internalId;

    }; // End of class AT24CXX_I2C

} // End of namespace _AT24CXX_I2C

using namespace _AT24CXX_I2C;

#endif // __AT24CXX_I2C_H__
