# IRPMon

The goal of the tool is to monitor requests received by selected device objects or kernel drivers. The tool is quite similar to IrpTracker but has several enhancements. It supports 64-bit versions of Windows (no inline hooks are used, only modifications to driver object structures are performed) and monitors IRP, FastIo, AddDevice, DriverUnload and StartIo requests.

## Compilation

### Drivers, Servers and DLLs

You need Visual Studio 2017 or 2019 to compile drivers and DLLs. All is set for the 2019, however, adopting the settings to 2017 one should be a piece of cake. In order to build the XP configuration (WIndows XP support), you need to install the Windows XP support into your Visual Studio. Also, install latest version of the Windows SDK and Windows Driver Kit (latest at the time of this writing at least). To use the `scripts\build.bat`, install MSBuild as well.

### GUI Application

Delphi 10.3 Rio does the trick here, although you should be able to compile the project also with Delphi XE2 and later. If you are looking for WIndows XP support, Lazarus seems to be your only option. Version 2.0.8 seems to produce pretty good results.

## Donations

If you wish to support development of this tool, you may donate some stuff to the following addresses:

* 17tEADhePvhHPj2X5GWn8vfiYhZCRH1f7V (BTC)
* [![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=6QWP4WH49Y3Z2&item_name=IRPMon&currency_code=CZK&source=url)
