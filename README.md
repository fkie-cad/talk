# Talk

Talks to a device using NtDeviceIoControl.


## Version
2.2.0  
Last changed: 12.12.2025

## Contents
* [Requirements](#requirements)
* [Build](#build)
* [Usage](#usage)
    * [Examples](#examples)
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
           [/is|/ir|/ip|/os|/or|/op <size>] 
           [/ipc|/opc <pattern> <size>] 
           [/i|o(x|b|w|d|q|a|u) <data>] 
           [/if|/of <file>] "
           [/s sleep] 
           [/da <flags>] 
           [/sa <flags>] 
           [/se <priv>] 
           [/t] 
           [/pb|pbs|pc8|pc16|pc32|pc64|pc1|pa|pu]
           [/v] 
           [/h]
```

**Options**
- /n DeviceName to call. I.e. "\Device\Beep"
- /c The desired IOCTL interpreted as a hex number.
- /os Size of OutputBuffer.

**Input Data**  
(The integer types are [chainable](#remarks).)  
- /ix \<data\> as hex byte string.
- /ib \<data\> as byte (uint8).
- /iw \<data\> as word (uint16).
- /id \<data\> as dword (uint32).
- /iq \<data\> as qword (uint64).
- /ia \<data\> as ascii text.
- /iu \<data\> as unicode (utf-16) text.
- /if Input data is read from the binary file from \<path\>.
- /ir Input data will be filled with \<size\> random bytes.
- /ip Input data will be filled with \<size\> default pattern bytes (Aa0Aa1...).
- /ipc Input data will be filled with \<size\> custom pattern bytes, starting from \<pattern\>, incremented by 1.
- /is Input data will be filled with \<size\> 'A's.

**Output Data:**  
(Sometimes the output buffer might need to be filled as well.)  
(The integer types are [chainable](#remarks).)  
- /os Size of OutputBuffer to be filled with zeros. (Most common option.)
- /ox \<data\> as hex byte string.
- /ob \<data\> as byte.
- /ow \<data\> as word (uint16).
- /od \<data\> as dword (uint32).
- /oq \<data\> as qword (uint64).
- /oa \<data\> as ascii text.
- /ou \<data\> as unicode (utf-16) text.
- /of Output data is read as binary data from the file <path>.
- /or Output data will be filled with <size> random bytes.
- /op Output data will be filled with <size> default pattern bytes (Aa0Aa1...).
- /opc Output data will be filled with <size> custom pattern bytes, starting from <pattern>, incremented by 1.


**Other**
- /s Duration of a possible sleep after device io.
- /t Just test the device for accessibility. Don't send data.
- /da DesiredAccess flags to open the device. Defaults to FILE_GENERIC_READ|FILE_GENERIC_WRITE|SYNCHRONIZE = 0x12019f
- /sa ShareAccess flags to open the device. Defaults to FILE_SHARE_READ|FILE_SHARE_WRITE = 0x3
- /se Additional SE_XXX privilege (if run as admin). Can be set multiple (0x10) times for multiple privileges.

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

### Remarks
A sleep (`/s`) may be useful with asynchronous calls like Beep.  

Integer arguments (`\ib`, `\iw`, `\id`, `\iq`, `\ob`, `\ow`, `\od`, `\oq`) can be chained together to form a simple struct.
This may sometimes be more convenient as to give a plain hex string.
Check the second beep example call to see an example of this. 
There the input struct would consist out of two ULONGs.
The resulting input size would be 8 bytes, equal to the first example.
The order the integers are given in does matter.

The custom `<pattern>` of `/ipc` (`/opc`) is interpreted as a byte string, 
i.e. the input of `/ipc 414243 10` will result in the input data of `41 42 43 41 42 44 41 42 45 41`.


### Examples
Call beep
```bash
# with a byte string
$ Talk.exe /n \Device\Beep /c 0x10000 /ix 020200003e080000 /s 0x083e
# with two ulongs
$ Talk.exe /n \Device\Beep /c 0x10000 /id 0x202 /id 0x83e /s 0x083e
```

Query Name and GUID of HarddiskVolume1 (Admin rights required) with different print options
```bash
$ Talk.exe /n \Device\HarddiskVolume1 /c 0x4d0008 /os 0x100 /pu
$ Talk.exe /n \Device\HarddiskVolume1 /c 0x4d0018 /os 0x10 /pb
```


## Copyright, Credits & Contact
Published under [GNU GENERAL PUBLIC LICENSE](LICENSE).
