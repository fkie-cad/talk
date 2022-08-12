# Talk
Talks to a device using NtDeviceIoControl.


## Version ##
2.0.5  
Last changed: 12.08.2022


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
$ Talk.exe /n DeviceName [/c ioctl] [/i InputBufferSize] [/o OutputBufferSize] [/d aabbcc] [/c ioctl] [/s SleepDuration] [/da <Flags>] [/sa <Flags>] [/t] [/h]
```

**Options**
 - /n DeviceName to call. I.e. "\Device\Beep"
 - /c The desired IOCTL.
 - /i InputBufferSize possible size of InputBuffer.
 - /o OutputBufferSize possible size of OutputBuffer.
 - /d InputBuffer data in hex.
 - /s Duration of sleep after call .
 - /t Just test the device for accessibility. Don't send data.
 - /da DesiredAccess flags to open the device. Defaults to FILE_GENERIC_READ|FILE_GENERIC_WRITE|SYNCHRONIZE = 0x12019f
 - /sa ShareAccess flags to open the device. Defaults to FILE_SHARE_READ|FILE_SHARE_WRITE =  0x3

**Remarks**  
If no input data (`/d`) but an input length (`/i`) is given, the buffer will be filled with As (0x41).  
A sleep (`/s`) may be useful with asynchronous calls like Beep.  
 
### EXAMPLE ###
Call beep
```bash
$ Talk.exe /n \Device\Beep /c 0x10000 /i 8 /d 020200003e080000 /s 0x083e
```


## COPYRIGHT, CREDITS & CONTACT ## 
Published under [GNU GENERAL PUBLIC LICENSE](LICENSE).

#### Author ####
- Viviane Zwanger ([viviane.zwanger@fkie.fraunhofer.de](viviane.zwanger@fkie.fraunhofer.de))

#### Author ####
- Henning Braun ([henning.braun@fkie.fraunhofer.de](henning.braun@fkie.fraunhofer.de)) 
