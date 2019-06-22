# IRPMon

The goal of the tool is to monitor requests received by selected device objects or kernel drivers. The tool is quite similar to IrpTracker but has several enhancements. It supports 64-bit versions of Windows (no inline hooks are used, only modifications to driver object structures are performed) and monitors IRP, FastIo, AddDevice, DriverUnload and StartIo requests.

## Compilation

* Visual Studio 2017 for drivers and DLLs,
* Delphi 10.3 Rio for the application (XE2 and newer should work too).


## Donations

If you wish to support development of this tool, you may donate some stuff (BTC) to the following address:

17tEADhePvhHPj2X5GWn8vfiYhZCRH1f7V