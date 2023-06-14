# Talk
Talks to a device using NtDeviceIoControl.


## Version ##
2.1.0  
Last changed: 25.04.2023


## REQUIREMENTS ##
- msbuild


## BUILD ##
```bash
$devcmd> build.bat [/?]
// or
$devcmd> msbuild talk.vcxproj /p:Platform=x64 /p:Configuration=Release
```

**other options**
```bash
$devcmd> msbuild [talk.vcxproj] [/p:Platform=x86|x64] [/p:Configuration=Debug|Release] [/p:RunTimeLib=Debug|Release] [/p:PDB=0|1]
```

## USAGE ##
```bash
$ Talk.exe /n DeviceName [/c <ioctl>] [/is <size>] [/os <size>] [/i(x|b|w|d|q|a|u|f) <data>] [/s sleep] [/da <flags>] [/sa <flags>] [/t] [/h]
```

**Options**
- /n DeviceName to call. I.e. "\Device\Beep"
- /c The desired IOCTL.
- /is Size of InputBuffer, if not filled with the data options. Will be filled with 'A's.
- /os Size of OutputBuffer.

**Input Data**
- /ix Input data as hex byte string.
- /ib Input data as byte.
- /iw Input data as word (uint16).
- /id Input data as dword (uint32).
- /iq Input data as qword (uint64).
- /ia Input data as ascii text.
- /iu Input data as unicode (utf-16) text.
- /if Input data from binary file.

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

**Remarks**  
If no input data (`/i*`) but an input length (`/is`) is given, the buffer will be filled with As (0x41).  
If input data (`/i*`) is given, the input length will be set to its size, independent of a size possibly set with `/is`.  
A sleep (`/s`) may be useful with asynchronous calls like Beep.  

### EXAMPLE ###
Call beep
```bash
$ Talk.exe /n \Device\Beep /c 0x10000 /ix 020200003e080000 /s 0x083e
```


## COPYRIGHT, CREDITS & CONTACT ## 
Published under [GNU GENERAL PUBLIC LICENSE](LICENSE).

#### Author ####
- Viviane Zwanger ([viviane.zwanger@fkie.fraunhofer.de](viviane.zwanger@fkie.fraunhofer.de))

#### Author ####
- Henning Braun ([henning.braun@fkie.fraunhofer.de](henning.braun@fkie.fraunhofer.de)) 
