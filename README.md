# Talk
Talks to a device using NtDeviceIoControl.


## Version ##
2.0.3  
Last changed: 12.10.2021


## REQUIREMENTS ##
- msbuild
- [wdk]

**Remarks**  
The .vcxproj file is using `WindowsApplicationForDrivers10.0` as the `PlatformToolset`, which leads to smaller builds. 
If the WDK is not installed, the `PlatformToolset` may be changed to `v142` and it should compile without errors.


## BUILD ##
```bash
$devcmd> msbuild talk.vcxproj /p:Platform=x64 /p:Configuration=Release
```

**other options**
```bash
$devcmd> msbuild [talk.vcxproj] [/p:Platform=x86|x64] [/p:Configuration=Debug|Release] [/p:RunTimeLib=Debug|Release] [/p:PDB=No]
```

## USAGE ##
```bash
$ Talk.exe /n DeviceName [/c ioctl] [/i InputBufferSize] [/o OutputBufferSize] [/d aabbcc] [/c ioctl] [/s SleepDuration] [/t] [/h]
```

**Options**
 - /n DeviceName to call. 
 - /c The desired IOCTL.
 - /i InputBufferSize possible size of InputBuffer.
 - /o OutputBufferSize possible size of OutputBuffer.
 - /d InputBuffer data in hex.
 - /s Duration of sleep after call .
 - /t Just test the device for accessibility. Don't send data.

**Remarks**  
If no input data (`/d`) but an input length (`/i`) is given, the buffer will be filled with As.  
A sleep (`/s`) may be useful with asynchronous calls like Beep.  
 
### EXAMPLE ###
```bash
$ Talk.exe /n Beep /c 0x10000 /i 8 /d 020200003e080000 /s 0x083e
```


## COPYRIGHT, CREDITS & CONTACT ## 
Published under [GNU GENERAL PUBLIC LICENSE](LICENSE).

#### Author ####
- Viviane Zwanger ([viviane.zwanger@fkie.fraunhofer.de](viviane.zwanger@fkie.fraunhofer.de))

#### Author ####
- Henning Braun ([henning.braun@fkie.fraunhofer.de](henning.braun@fkie.fraunhofer.de)) 
