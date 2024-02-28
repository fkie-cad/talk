# Talk

Talks to a device using NtDeviceIoControl.


## Version
2.1.6  
Last changed: 28.02.2024

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
           [/is|/ir|/ip <size>]
           [/ipc <start> <size>]
           [/i(x|b|w|d|q|a|u) <data>]
           [/s sleep] 
           [/da <flags>] 
           [/sa <flags>] 
           [/t] 
           [/pb|pbs|pc8|pc16|pc32|pc64|pc1|pa|pu]
           [/v] 
           [/h]
```

**Options**
- /n DeviceName to call. I.e. "\Device\Beep"
- /c The desired IOCTL in hex.
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
- /ip Input data will be filled with \<size\> pattern bytes (Aa0Aa1...).\n");
- /ipc Input data will be filled with \<size\> custom pattern bytes, starting from \<start\>.\n");
- /is Input data will be filled with \<size\> 'A's.

 **Other**
- /s Duration of a possible sleep after device io.
- /t Just test the device for accessibility. Don't send data.
- /da DesiredAccess flags to open the device. Defaults to FILE_GENERIC_READ|FILE_GENERIC_WRITE|SYNCHRONIZE = 0x12019f
- /sa ShareAccess flags to open the device. Defaults to FILE_SHARE_READ|FILE_SHARE_WRITE =  0x3

 **Printing style for output buffer**
- /pb Print plain space separated bytes
- /pbs Print plain byte string
- /pc8 Print in cols of Address | bytes | ascii chars
- /pc16 Print in cols of Address | words | utf-16 chars
- /pc32 Print in cols of Address | dwords
- /pc64 Print in cols of Address | qwords
- /pc1 Print in cols of Address | bits
- /pa Print as ascii string
- /pu Print as unicode (utf-16) string

**Misc**
- /v More verbose output.

**Remarks**  
A sleep (`/s`) may be useful with asynchronous calls like Beep.  
Custom patterns (`/ipc`) are auto aligned to 1, 2, 4 or 8 bytes.  


### Example
Call beep
```bash
$ Talk.exe /n \Device\Beep /c 0x10000 /ix 020200003e080000 /s 0x083e
```

Query Name and GUID of HarddiskVolume1
```bash
$ Talk.exe /n \Device\HarddiskVolume1 /c 0x4d0008 /os 0x100 /pu
$ Talk.exe /n \Device\HarddiskVolume1 /c 0x4d0018 /os 0x10 /pb
```


## Copyright, Credits & Contact
Published under [GNU GENERAL PUBLIC LICENSE](LICENSE).

### Authors
- Viviane Zwanger ([viviane.zwanger@fkie.fraunhofer.de](mailto:viviane.zwanger@fkie.fraunhofer.de))
- Henning Braun ([henning.braun@fkie.fraunhofer.de](mailto:henning.braun@fkie.fraunhofer.de)) 
