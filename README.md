# Talk

Talks to a device using NtDeviceIoControl.


## Version
2.1.3  
Last changed: 19.01.2024

## Contents
* [Requirements](#requirements)
* [Build](#build)
* [Usage](#usage)
    * [Example](#example)
* [Copyright, Credits & Contact](#copyright,-credits-&-contact)
    * [Authors](#authors)


## Requirements
- msbuild


## Build
```bash
$devcmd> build.bat [/?]
// or
$devcmd> msbuild talk.vcxproj /p:Platform=x64 /p:Configuration=Release
```

**other options**
```bash
$devcmd> msbuild [talk.vcxproj] [/p:Platform=x86|x64] [/p:Configuration=Debug|Release] [/p:RunTimeLib=Debug|Release] [/p:PDB=0|1]
```

## Usage
```bash
$ Talk.exe /n DeviceName 
           [/c <ioctl>] 
           [/os <size>] 
           [/i(x|b|w|d|q|a|u) <data> | /if <file> | /is|/ir <size>] 
           [/s sleep] 
           [/da <flags>] 
           [/sa <flags>] 
           [/t] 
           [/pb|pc8|pc16|pc32|pc64|pcy]
           [/v] 
           [/h]
```

**Options**
- /n DeviceName to call. I.e. "\Device\Beep"
- /c The desired IOCTL.
- /os Size of OutputBuffer.

**Input Data**
- /ix Input data as hex byte string.
- /ib Input data as byte.
- /iw Input data as word (uint16).
- /id Input data as dword (uint32).
- /iq Input data as qword (uint64).
- /ia Input data as ascii text.
- /iu Input data as unicode (utf-16) text.
- /if Input data is read from the binary file from \<path\>.
- /ir Input data will be filled with \<size\> random bytes.
- /is Input data will be filled with \<size\> 'A's.

 **Other**
- /s Duration of a possible sleep after device io.
- /t Just test the device for accessibility. Don't send data.
- /da DesiredAccess flags to open the device. Defaults to FILE_GENERIC_READ|FILE_GENERIC_WRITE|SYNCHRONIZE = 0x12019f
- /sa ShareAccess flags to open the device. Defaults to FILE_SHARE_READ|FILE_SHARE_WRITE =  0x3

 **Printing style for output buffer**
- /pb Print in plain bytes
- /pc8 Print in cols of Address | bytes | ascii chars
- /pc16 Print in cols of Address | words | utf-16 chars
- /pc32 Print in cols of Address | dwords
- /pc64 Print in cols of Address | qwords
- /pcy Print in cols of Address | bits

**Misc**
- /v More verbose output.

**Remarks**  
A sleep (`/s`) may be useful with asynchronous calls like Beep.  


### Example
Call beep
```bash
$ Talk.exe /n \Device\Beep /c 0x10000 /ix 020200003e080000 /s 0x083e
```


## Copyright, Credits & Contact
Published under [GNU GENERAL PUBLIC LICENSE](LICENSE).

### Authors
- Viviane Zwanger ([viviane.zwanger@fkie.fraunhofer.de](mailto:viviane.zwanger@fkie.fraunhofer.de))
- Henning Braun ([henning.braun@fkie.fraunhofer.de](mailto:henning.braun@fkie.fraunhofer.de)) 
